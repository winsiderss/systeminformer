/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* * Mips Disassembler                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* Capstone Disassembly Engine, http://www.capstone-engine.org */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#include "../../MCInst.h"
#include "../../LEB128.h"

// Helper function for extracting fields from encoded instructions.
#define FieldFromInstruction(fname, InsnType) \
static InsnType fname(InsnType insn, unsigned startBit, unsigned numBits) \
{ \
  InsnType fieldMask; \
  if (numBits == sizeof(InsnType)*8) \
    fieldMask = (InsnType)(-1LL); \
  else \
    fieldMask = (((InsnType)1 << numBits) - 1) << startBit; \
  return (insn & fieldMask) >> startBit; \
}

static uint8_t DecoderTableCOP3_32[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 51, 8, 0, // Skip to: 15
/* 7 */       MCD_OPC_CheckPredicate, 1, 40, 0, // Skip to: 51
/* 11 */      MCD_OPC_Decode, 220, 7, 10, // Opcode: LWC3
/* 15 */      MCD_OPC_FilterValue, 55, 8, 0, // Skip to: 27
/* 19 */      MCD_OPC_CheckPredicate, 2, 28, 0, // Skip to: 51
/* 23 */      MCD_OPC_Decode, 167, 7, 10, // Opcode: LDC3
/* 27 */      MCD_OPC_FilterValue, 59, 8, 0, // Skip to: 39
/* 31 */      MCD_OPC_CheckPredicate, 1, 16, 0, // Skip to: 51
/* 35 */      MCD_OPC_Decode, 242, 12, 10, // Opcode: SWC3
/* 39 */      MCD_OPC_FilterValue, 63, 8, 0, // Skip to: 51
/* 43 */      MCD_OPC_CheckPredicate, 2, 4, 0, // Skip to: 51
/* 47 */      MCD_OPC_Decode, 161, 11, 10, // Opcode: SDC3
/* 51 */      MCD_OPC_Fail,
  0
};

static uint8_t DecoderTableMicroMips16[] = {
/* 0 */       MCD_OPC_ExtractField, 10, 6,  // Inst{15-10} ...
/* 3 */       MCD_OPC_FilterValue, 1, 26, 0, // Skip to: 33
/* 7 */       MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 10 */      MCD_OPC_FilterValue, 0, 7, 0, // Skip to: 21
/* 14 */      MCD_OPC_CheckPredicate, 3, 19, 2, // Skip to: 549
/* 18 */      MCD_OPC_Decode, 52, 11, // Opcode: ADDU16_MM
/* 21 */      MCD_OPC_FilterValue, 1, 12, 2, // Skip to: 549
/* 25 */      MCD_OPC_CheckPredicate, 3, 8, 2, // Skip to: 549
/* 29 */      MCD_OPC_Decode, 214, 12, 11, // Opcode: SUBU16_MM
/* 33 */      MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 45
/* 37 */      MCD_OPC_CheckPredicate, 3, 252, 1, // Skip to: 549
/* 41 */      MCD_OPC_Decode, 155, 7, 12, // Opcode: LBU16_MM
/* 45 */      MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 57
/* 49 */      MCD_OPC_CheckPredicate, 3, 240, 1, // Skip to: 549
/* 53 */      MCD_OPC_Decode, 233, 8, 13, // Opcode: MOVE16_MM
/* 57 */      MCD_OPC_FilterValue, 9, 27, 0, // Skip to: 88
/* 61 */      MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 64 */      MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 76
/* 68 */      MCD_OPC_CheckPredicate, 3, 221, 1, // Skip to: 549
/* 72 */      MCD_OPC_Decode, 226, 11, 14, // Opcode: SLL16_MM
/* 76 */      MCD_OPC_FilterValue, 1, 213, 1, // Skip to: 549
/* 80 */      MCD_OPC_CheckPredicate, 3, 209, 1, // Skip to: 549
/* 84 */      MCD_OPC_Decode, 160, 12, 14, // Opcode: SRL16_MM
/* 88 */      MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 100
/* 92 */      MCD_OPC_CheckPredicate, 3, 197, 1, // Skip to: 549
/* 96 */      MCD_OPC_Decode, 186, 7, 12, // Opcode: LHU16_MM
/* 100 */     MCD_OPC_FilterValue, 11, 7, 0, // Skip to: 111
/* 104 */     MCD_OPC_CheckPredicate, 3, 185, 1, // Skip to: 549
/* 108 */     MCD_OPC_Decode, 86, 15, // Opcode: ANDI16_MM
/* 111 */     MCD_OPC_FilterValue, 17, 226, 0, // Skip to: 341
/* 115 */     MCD_OPC_ExtractField, 6, 4,  // Inst{9-6} ...
/* 118 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 130
/* 122 */     MCD_OPC_CheckPredicate, 3, 167, 1, // Skip to: 549
/* 126 */     MCD_OPC_Decode, 130, 10, 16, // Opcode: NOT16_MM
/* 130 */     MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 142
/* 134 */     MCD_OPC_CheckPredicate, 3, 155, 1, // Skip to: 549
/* 138 */     MCD_OPC_Decode, 237, 13, 17, // Opcode: XOR16_MM
/* 142 */     MCD_OPC_FilterValue, 2, 7, 0, // Skip to: 153
/* 146 */     MCD_OPC_CheckPredicate, 3, 143, 1, // Skip to: 549
/* 150 */     MCD_OPC_Decode, 84, 17, // Opcode: AND16_MM
/* 153 */     MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 165
/* 157 */     MCD_OPC_CheckPredicate, 3, 132, 1, // Skip to: 549
/* 161 */     MCD_OPC_Decode, 134, 10, 17, // Opcode: OR16_MM
/* 165 */     MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 177
/* 169 */     MCD_OPC_CheckPredicate, 3, 120, 1, // Skip to: 549
/* 173 */     MCD_OPC_Decode, 225, 7, 18, // Opcode: LWM16_MM
/* 177 */     MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 189
/* 181 */     MCD_OPC_CheckPredicate, 3, 108, 1, // Skip to: 549
/* 185 */     MCD_OPC_Decode, 246, 12, 18, // Opcode: SWM16_MM
/* 189 */     MCD_OPC_FilterValue, 6, 27, 0, // Skip to: 220
/* 193 */     MCD_OPC_ExtractField, 5, 1,  // Inst{5} ...
/* 196 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 208
/* 200 */     MCD_OPC_CheckPredicate, 3, 89, 1, // Skip to: 549
/* 204 */     MCD_OPC_Decode, 137, 7, 19, // Opcode: JR16_MM
/* 208 */     MCD_OPC_FilterValue, 1, 81, 1, // Skip to: 549
/* 212 */     MCD_OPC_CheckPredicate, 3, 77, 1, // Skip to: 549
/* 216 */     MCD_OPC_Decode, 140, 7, 19, // Opcode: JRC16_MM
/* 220 */     MCD_OPC_FilterValue, 7, 27, 0, // Skip to: 251
/* 224 */     MCD_OPC_ExtractField, 5, 1,  // Inst{5} ...
/* 227 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 239
/* 231 */     MCD_OPC_CheckPredicate, 3, 58, 1, // Skip to: 549
/* 235 */     MCD_OPC_Decode, 250, 6, 19, // Opcode: JALR16_MM
/* 239 */     MCD_OPC_FilterValue, 1, 50, 1, // Skip to: 549
/* 243 */     MCD_OPC_CheckPredicate, 3, 46, 1, // Skip to: 549
/* 247 */     MCD_OPC_Decode, 254, 6, 19, // Opcode: JALRS16_MM
/* 251 */     MCD_OPC_FilterValue, 8, 14, 0, // Skip to: 269
/* 255 */     MCD_OPC_CheckPredicate, 3, 34, 1, // Skip to: 549
/* 259 */     MCD_OPC_CheckField, 5, 1, 0, 28, 1, // Skip to: 549
/* 265 */     MCD_OPC_Decode, 187, 8, 19, // Opcode: MFHI16_MM
/* 269 */     MCD_OPC_FilterValue, 9, 14, 0, // Skip to: 287
/* 273 */     MCD_OPC_CheckPredicate, 3, 16, 1, // Skip to: 549
/* 277 */     MCD_OPC_CheckField, 5, 1, 0, 10, 1, // Skip to: 549
/* 283 */     MCD_OPC_Decode, 192, 8, 19, // Opcode: MFLO16_MM
/* 287 */     MCD_OPC_FilterValue, 10, 14, 0, // Skip to: 305
/* 291 */     MCD_OPC_CheckPredicate, 3, 254, 0, // Skip to: 549
/* 295 */     MCD_OPC_CheckField, 4, 2, 0, 248, 0, // Skip to: 549
/* 301 */     MCD_OPC_Decode, 172, 2, 20, // Opcode: BREAK16_MM
/* 305 */     MCD_OPC_FilterValue, 11, 14, 0, // Skip to: 323
/* 309 */     MCD_OPC_CheckPredicate, 3, 236, 0, // Skip to: 549
/* 313 */     MCD_OPC_CheckField, 4, 2, 0, 230, 0, // Skip to: 549
/* 319 */     MCD_OPC_Decode, 153, 11, 20, // Opcode: SDBBP16_MM
/* 323 */     MCD_OPC_FilterValue, 12, 222, 0, // Skip to: 549
/* 327 */     MCD_OPC_CheckPredicate, 3, 218, 0, // Skip to: 549
/* 331 */     MCD_OPC_CheckField, 5, 1, 0, 212, 0, // Skip to: 549
/* 337 */     MCD_OPC_Decode, 139, 7, 21, // Opcode: JRADDIUSP
/* 341 */     MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 353
/* 345 */     MCD_OPC_CheckPredicate, 3, 200, 0, // Skip to: 549
/* 349 */     MCD_OPC_Decode, 233, 7, 22, // Opcode: LWSP_MM
/* 353 */     MCD_OPC_FilterValue, 19, 25, 0, // Skip to: 382
/* 357 */     MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 360 */     MCD_OPC_FilterValue, 0, 7, 0, // Skip to: 371
/* 364 */     MCD_OPC_CheckPredicate, 3, 181, 0, // Skip to: 549
/* 368 */     MCD_OPC_Decode, 30, 23, // Opcode: ADDIUS5_MM
/* 371 */     MCD_OPC_FilterValue, 1, 174, 0, // Skip to: 549
/* 375 */     MCD_OPC_CheckPredicate, 3, 170, 0, // Skip to: 549
/* 379 */     MCD_OPC_Decode, 31, 24, // Opcode: ADDIUSP_MM
/* 382 */     MCD_OPC_FilterValue, 25, 8, 0, // Skip to: 394
/* 386 */     MCD_OPC_CheckPredicate, 3, 159, 0, // Skip to: 549
/* 390 */     MCD_OPC_Decode, 221, 7, 25, // Opcode: LWGP_MM
/* 394 */     MCD_OPC_FilterValue, 26, 8, 0, // Skip to: 406
/* 398 */     MCD_OPC_CheckPredicate, 3, 147, 0, // Skip to: 549
/* 402 */     MCD_OPC_Decode, 214, 7, 12, // Opcode: LW16_MM
/* 406 */     MCD_OPC_FilterValue, 27, 25, 0, // Skip to: 435
/* 410 */     MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 413 */     MCD_OPC_FilterValue, 0, 7, 0, // Skip to: 424
/* 417 */     MCD_OPC_CheckPredicate, 3, 128, 0, // Skip to: 549
/* 421 */     MCD_OPC_Decode, 29, 26, // Opcode: ADDIUR2_MM
/* 424 */     MCD_OPC_FilterValue, 1, 121, 0, // Skip to: 549
/* 428 */     MCD_OPC_CheckPredicate, 3, 117, 0, // Skip to: 549
/* 432 */     MCD_OPC_Decode, 28, 27, // Opcode: ADDIUR1SP_MM
/* 435 */     MCD_OPC_FilterValue, 33, 14, 0, // Skip to: 453
/* 439 */     MCD_OPC_CheckPredicate, 3, 106, 0, // Skip to: 549
/* 443 */     MCD_OPC_CheckField, 0, 1, 0, 100, 0, // Skip to: 549
/* 449 */     MCD_OPC_Decode, 234, 8, 28, // Opcode: MOVEP_MM
/* 453 */     MCD_OPC_FilterValue, 34, 8, 0, // Skip to: 465
/* 457 */     MCD_OPC_CheckPredicate, 3, 88, 0, // Skip to: 549
/* 461 */     MCD_OPC_Decode, 143, 11, 12, // Opcode: SB16_MM
/* 465 */     MCD_OPC_FilterValue, 35, 8, 0, // Skip to: 477
/* 469 */     MCD_OPC_CheckPredicate, 3, 76, 0, // Skip to: 549
/* 473 */     MCD_OPC_Decode, 210, 1, 29, // Opcode: BEQZ16_MM
/* 477 */     MCD_OPC_FilterValue, 42, 8, 0, // Skip to: 489
/* 481 */     MCD_OPC_CheckPredicate, 3, 64, 0, // Skip to: 549
/* 485 */     MCD_OPC_Decode, 187, 11, 12, // Opcode: SH16_MM
/* 489 */     MCD_OPC_FilterValue, 43, 8, 0, // Skip to: 501
/* 493 */     MCD_OPC_CheckPredicate, 3, 52, 0, // Skip to: 549
/* 497 */     MCD_OPC_Decode, 157, 2, 29, // Opcode: BNEZ16_MM
/* 501 */     MCD_OPC_FilterValue, 50, 8, 0, // Skip to: 513
/* 505 */     MCD_OPC_CheckPredicate, 3, 40, 0, // Skip to: 549
/* 509 */     MCD_OPC_Decode, 253, 12, 22, // Opcode: SWSP_MM
/* 513 */     MCD_OPC_FilterValue, 51, 8, 0, // Skip to: 525
/* 517 */     MCD_OPC_CheckPredicate, 4, 28, 0, // Skip to: 549
/* 521 */     MCD_OPC_Decode, 165, 1, 30, // Opcode: B16_MM
/* 525 */     MCD_OPC_FilterValue, 58, 8, 0, // Skip to: 537
/* 529 */     MCD_OPC_CheckPredicate, 3, 16, 0, // Skip to: 549
/* 533 */     MCD_OPC_Decode, 236, 12, 12, // Opcode: SW16_MM
/* 537 */     MCD_OPC_FilterValue, 59, 8, 0, // Skip to: 549
/* 541 */     MCD_OPC_CheckPredicate, 3, 4, 0, // Skip to: 549
/* 545 */     MCD_OPC_Decode, 192, 7, 31, // Opcode: LI16_MM
/* 549 */     MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTableMicroMips32[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 0, 189, 3, // Skip to: 964
/* 7 */       MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 10 */      MCD_OPC_FilterValue, 0, 90, 0, // Skip to: 104
/* 14 */      MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 17 */      MCD_OPC_FilterValue, 0, 47, 0, // Skip to: 68
/* 21 */      MCD_OPC_ExtractField, 11, 15,  // Inst{25-11} ...
/* 24 */      MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 36
/* 28 */      MCD_OPC_CheckPredicate, 3, 28, 0, // Skip to: 60
/* 32 */      MCD_OPC_Decode, 181, 12, 0, // Opcode: SSNOP_MM
/* 36 */      MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 48
/* 40 */      MCD_OPC_CheckPredicate, 3, 16, 0, // Skip to: 60
/* 44 */      MCD_OPC_Decode, 140, 5, 0, // Opcode: EHB_MM
/* 48 */      MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 60
/* 52 */      MCD_OPC_CheckPredicate, 3, 4, 0, // Skip to: 60
/* 56 */      MCD_OPC_Decode, 148, 10, 0, // Opcode: PAUSE_MM
/* 60 */      MCD_OPC_CheckPredicate, 3, 38, 6, // Skip to: 1638
/* 64 */      MCD_OPC_Decode, 238, 11, 32, // Opcode: SLL_MM
/* 68 */      MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 80
/* 72 */      MCD_OPC_CheckPredicate, 3, 26, 6, // Skip to: 1638
/* 76 */      MCD_OPC_Decode, 178, 12, 32, // Opcode: SRL_MM
/* 80 */      MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 92
/* 84 */      MCD_OPC_CheckPredicate, 3, 14, 6, // Skip to: 1638
/* 88 */      MCD_OPC_Decode, 157, 12, 32, // Opcode: SRA_MM
/* 92 */      MCD_OPC_FilterValue, 3, 6, 6, // Skip to: 1638
/* 96 */      MCD_OPC_CheckPredicate, 3, 2, 6, // Skip to: 1638
/* 100 */     MCD_OPC_Decode, 250, 10, 32, // Opcode: ROTR_MM
/* 104 */     MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 116
/* 108 */     MCD_OPC_CheckPredicate, 3, 246, 5, // Skip to: 1638
/* 112 */     MCD_OPC_Decode, 173, 2, 33, // Opcode: BREAK_MM
/* 116 */     MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 128
/* 120 */     MCD_OPC_CheckPredicate, 3, 234, 5, // Skip to: 1638
/* 124 */     MCD_OPC_Decode, 246, 6, 34, // Opcode: INS_MM
/* 128 */     MCD_OPC_FilterValue, 16, 180, 0, // Skip to: 312
/* 132 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 135 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 147
/* 139 */     MCD_OPC_CheckPredicate, 3, 215, 5, // Skip to: 1638
/* 143 */     MCD_OPC_Decode, 234, 11, 35, // Opcode: SLLV_MM
/* 147 */     MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 159
/* 151 */     MCD_OPC_CheckPredicate, 3, 203, 5, // Skip to: 1638
/* 155 */     MCD_OPC_Decode, 174, 12, 35, // Opcode: SRLV_MM
/* 159 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 171
/* 163 */     MCD_OPC_CheckPredicate, 3, 191, 5, // Skip to: 1638
/* 167 */     MCD_OPC_Decode, 153, 12, 35, // Opcode: SRAV_MM
/* 171 */     MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 183
/* 175 */     MCD_OPC_CheckPredicate, 3, 179, 5, // Skip to: 1638
/* 179 */     MCD_OPC_Decode, 249, 10, 35, // Opcode: ROTRV_MM
/* 183 */     MCD_OPC_FilterValue, 4, 7, 0, // Skip to: 194
/* 187 */     MCD_OPC_CheckPredicate, 3, 167, 5, // Skip to: 1638
/* 191 */     MCD_OPC_Decode, 72, 36, // Opcode: ADD_MM
/* 194 */     MCD_OPC_FilterValue, 5, 7, 0, // Skip to: 205
/* 198 */     MCD_OPC_CheckPredicate, 3, 156, 5, // Skip to: 1638
/* 202 */     MCD_OPC_Decode, 78, 36, // Opcode: ADDu_MM
/* 205 */     MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 217
/* 209 */     MCD_OPC_CheckPredicate, 3, 145, 5, // Skip to: 1638
/* 213 */     MCD_OPC_Decode, 229, 12, 36, // Opcode: SUB_MM
/* 217 */     MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 229
/* 221 */     MCD_OPC_CheckPredicate, 3, 133, 5, // Skip to: 1638
/* 225 */     MCD_OPC_Decode, 231, 12, 36, // Opcode: SUBu_MM
/* 229 */     MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 241
/* 233 */     MCD_OPC_CheckPredicate, 3, 121, 5, // Skip to: 1638
/* 237 */     MCD_OPC_Decode, 217, 9, 36, // Opcode: MUL_MM
/* 241 */     MCD_OPC_FilterValue, 9, 7, 0, // Skip to: 252
/* 245 */     MCD_OPC_CheckPredicate, 3, 109, 5, // Skip to: 1638
/* 249 */     MCD_OPC_Decode, 88, 36, // Opcode: AND_MM
/* 252 */     MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 264
/* 256 */     MCD_OPC_CheckPredicate, 3, 98, 5, // Skip to: 1638
/* 260 */     MCD_OPC_Decode, 137, 10, 36, // Opcode: OR_MM
/* 264 */     MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 276
/* 268 */     MCD_OPC_CheckPredicate, 3, 86, 5, // Skip to: 1638
/* 272 */     MCD_OPC_Decode, 253, 9, 36, // Opcode: NOR_MM
/* 276 */     MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 288
/* 280 */     MCD_OPC_CheckPredicate, 3, 74, 5, // Skip to: 1638
/* 284 */     MCD_OPC_Decode, 240, 13, 36, // Opcode: XOR_MM
/* 288 */     MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 300
/* 292 */     MCD_OPC_CheckPredicate, 3, 62, 5, // Skip to: 1638
/* 296 */     MCD_OPC_Decode, 242, 11, 36, // Opcode: SLT_MM
/* 300 */     MCD_OPC_FilterValue, 14, 54, 5, // Skip to: 1638
/* 304 */     MCD_OPC_CheckPredicate, 3, 50, 5, // Skip to: 1638
/* 308 */     MCD_OPC_Decode, 251, 11, 36, // Opcode: SLTu_MM
/* 312 */     MCD_OPC_FilterValue, 24, 39, 0, // Skip to: 355
/* 316 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 319 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 331
/* 323 */     MCD_OPC_CheckPredicate, 3, 31, 5, // Skip to: 1638
/* 327 */     MCD_OPC_Decode, 253, 8, 37, // Opcode: MOVN_I_MM
/* 331 */     MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 343
/* 335 */     MCD_OPC_CheckPredicate, 3, 19, 5, // Skip to: 1638
/* 339 */     MCD_OPC_Decode, 145, 9, 37, // Opcode: MOVZ_I_MM
/* 343 */     MCD_OPC_FilterValue, 4, 11, 5, // Skip to: 1638
/* 347 */     MCD_OPC_CheckPredicate, 3, 7, 5, // Skip to: 1638
/* 351 */     MCD_OPC_Decode, 239, 7, 38, // Opcode: LWXS_MM
/* 355 */     MCD_OPC_FilterValue, 44, 8, 0, // Skip to: 367
/* 359 */     MCD_OPC_CheckPredicate, 3, 251, 4, // Skip to: 1638
/* 363 */     MCD_OPC_Decode, 160, 5, 39, // Opcode: EXT_MM
/* 367 */     MCD_OPC_FilterValue, 60, 243, 4, // Skip to: 1638
/* 371 */     MCD_OPC_ExtractField, 6, 6,  // Inst{11-6} ...
/* 374 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 386
/* 378 */     MCD_OPC_CheckPredicate, 3, 232, 4, // Skip to: 1638
/* 382 */     MCD_OPC_Decode, 185, 13, 40, // Opcode: TEQ_MM
/* 386 */     MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 398
/* 390 */     MCD_OPC_CheckPredicate, 3, 220, 4, // Skip to: 1638
/* 394 */     MCD_OPC_Decode, 193, 13, 40, // Opcode: TGE_MM
/* 398 */     MCD_OPC_FilterValue, 13, 123, 0, // Skip to: 525
/* 402 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 405 */     MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 423
/* 409 */     MCD_OPC_CheckPredicate, 3, 201, 4, // Skip to: 1638
/* 413 */     MCD_OPC_CheckField, 16, 10, 0, 195, 4, // Skip to: 1638
/* 419 */     MCD_OPC_Decode, 195, 13, 0, // Opcode: TLBP_MM
/* 423 */     MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 441
/* 427 */     MCD_OPC_CheckPredicate, 3, 183, 4, // Skip to: 1638
/* 431 */     MCD_OPC_CheckField, 16, 10, 0, 177, 4, // Skip to: 1638
/* 437 */     MCD_OPC_Decode, 197, 13, 0, // Opcode: TLBR_MM
/* 441 */     MCD_OPC_FilterValue, 2, 14, 0, // Skip to: 459
/* 445 */     MCD_OPC_CheckPredicate, 3, 165, 4, // Skip to: 1638
/* 449 */     MCD_OPC_CheckField, 16, 10, 0, 159, 4, // Skip to: 1638
/* 455 */     MCD_OPC_Decode, 199, 13, 0, // Opcode: TLBWI_MM
/* 459 */     MCD_OPC_FilterValue, 3, 14, 0, // Skip to: 477
/* 463 */     MCD_OPC_CheckPredicate, 3, 147, 4, // Skip to: 1638
/* 467 */     MCD_OPC_CheckField, 16, 10, 0, 141, 4, // Skip to: 1638
/* 473 */     MCD_OPC_Decode, 201, 13, 0, // Opcode: TLBWR_MM
/* 477 */     MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 489
/* 481 */     MCD_OPC_CheckPredicate, 3, 129, 4, // Skip to: 1638
/* 485 */     MCD_OPC_Decode, 232, 13, 41, // Opcode: WAIT_MM
/* 489 */     MCD_OPC_FilterValue, 14, 14, 0, // Skip to: 507
/* 493 */     MCD_OPC_CheckPredicate, 3, 117, 4, // Skip to: 1638
/* 497 */     MCD_OPC_CheckField, 16, 10, 0, 111, 4, // Skip to: 1638
/* 503 */     MCD_OPC_Decode, 175, 4, 0, // Opcode: DERET_MM
/* 507 */     MCD_OPC_FilterValue, 15, 103, 4, // Skip to: 1638
/* 511 */     MCD_OPC_CheckPredicate, 3, 99, 4, // Skip to: 1638
/* 515 */     MCD_OPC_CheckField, 16, 10, 0, 93, 4, // Skip to: 1638
/* 521 */     MCD_OPC_Decode, 144, 5, 0, // Opcode: ERET_MM
/* 525 */     MCD_OPC_FilterValue, 16, 8, 0, // Skip to: 537
/* 529 */     MCD_OPC_CheckPredicate, 3, 81, 4, // Skip to: 1638
/* 533 */     MCD_OPC_Decode, 192, 13, 40, // Opcode: TGEU_MM
/* 537 */     MCD_OPC_FilterValue, 29, 39, 0, // Skip to: 580
/* 541 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 544 */     MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 562
/* 548 */     MCD_OPC_CheckPredicate, 3, 62, 4, // Skip to: 1638
/* 552 */     MCD_OPC_CheckField, 21, 5, 0, 56, 4, // Skip to: 1638
/* 558 */     MCD_OPC_Decode, 193, 4, 42, // Opcode: DI_MM
/* 562 */     MCD_OPC_FilterValue, 5, 48, 4, // Skip to: 1638
/* 566 */     MCD_OPC_CheckPredicate, 3, 44, 4, // Skip to: 1638
/* 570 */     MCD_OPC_CheckField, 21, 5, 0, 38, 4, // Skip to: 1638
/* 576 */     MCD_OPC_Decode, 142, 5, 42, // Opcode: EI_MM
/* 580 */     MCD_OPC_FilterValue, 32, 8, 0, // Skip to: 592
/* 584 */     MCD_OPC_CheckPredicate, 3, 26, 4, // Skip to: 1638
/* 588 */     MCD_OPC_Decode, 208, 13, 40, // Opcode: TLT_MM
/* 592 */     MCD_OPC_FilterValue, 40, 8, 0, // Skip to: 604
/* 596 */     MCD_OPC_CheckPredicate, 3, 14, 4, // Skip to: 1638
/* 600 */     MCD_OPC_Decode, 207, 13, 40, // Opcode: TLTU_MM
/* 604 */     MCD_OPC_FilterValue, 44, 171, 0, // Skip to: 779
/* 608 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 611 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 623
/* 615 */     MCD_OPC_CheckPredicate, 3, 251, 3, // Skip to: 1638
/* 619 */     MCD_OPC_Decode, 170, 11, 43, // Opcode: SEB_MM
/* 623 */     MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 635
/* 627 */     MCD_OPC_CheckPredicate, 3, 239, 3, // Skip to: 1638
/* 631 */     MCD_OPC_Decode, 173, 11, 43, // Opcode: SEH_MM
/* 635 */     MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 647
/* 639 */     MCD_OPC_CheckPredicate, 3, 227, 3, // Skip to: 1638
/* 643 */     MCD_OPC_Decode, 134, 3, 43, // Opcode: CLO_MM
/* 647 */     MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 659
/* 651 */     MCD_OPC_CheckPredicate, 3, 215, 3, // Skip to: 1638
/* 655 */     MCD_OPC_Decode, 153, 3, 43, // Opcode: CLZ_MM
/* 659 */     MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 671
/* 663 */     MCD_OPC_CheckPredicate, 3, 203, 3, // Skip to: 1638
/* 667 */     MCD_OPC_Decode, 240, 10, 44, // Opcode: RDHWR_MM
/* 671 */     MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 683
/* 675 */     MCD_OPC_CheckPredicate, 3, 191, 3, // Skip to: 1638
/* 679 */     MCD_OPC_Decode, 235, 13, 43, // Opcode: WSBH_MM
/* 683 */     MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 695
/* 687 */     MCD_OPC_CheckPredicate, 3, 179, 3, // Skip to: 1638
/* 691 */     MCD_OPC_Decode, 209, 9, 45, // Opcode: MULT_MM
/* 695 */     MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 707
/* 699 */     MCD_OPC_CheckPredicate, 3, 167, 3, // Skip to: 1638
/* 703 */     MCD_OPC_Decode, 211, 9, 45, // Opcode: MULTu_MM
/* 707 */     MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 719
/* 711 */     MCD_OPC_CheckPredicate, 3, 155, 3, // Skip to: 1638
/* 715 */     MCD_OPC_Decode, 163, 11, 45, // Opcode: SDIV_MM
/* 719 */     MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 731
/* 723 */     MCD_OPC_CheckPredicate, 3, 143, 3, // Skip to: 1638
/* 727 */     MCD_OPC_Decode, 223, 13, 45, // Opcode: UDIV_MM
/* 731 */     MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 743
/* 735 */     MCD_OPC_CheckPredicate, 3, 131, 3, // Skip to: 1638
/* 739 */     MCD_OPC_Decode, 146, 8, 45, // Opcode: MADD_MM
/* 743 */     MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 755
/* 747 */     MCD_OPC_CheckPredicate, 3, 119, 3, // Skip to: 1638
/* 751 */     MCD_OPC_Decode, 137, 8, 45, // Opcode: MADDU_MM
/* 755 */     MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 767
/* 759 */     MCD_OPC_CheckPredicate, 3, 107, 3, // Skip to: 1638
/* 763 */     MCD_OPC_Decode, 164, 9, 45, // Opcode: MSUB_MM
/* 767 */     MCD_OPC_FilterValue, 15, 99, 3, // Skip to: 1638
/* 771 */     MCD_OPC_CheckPredicate, 3, 95, 3, // Skip to: 1638
/* 775 */     MCD_OPC_Decode, 155, 9, 45, // Opcode: MSUBU_MM
/* 779 */     MCD_OPC_FilterValue, 45, 45, 0, // Skip to: 828
/* 783 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 786 */     MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 804
/* 790 */     MCD_OPC_CheckPredicate, 3, 76, 3, // Skip to: 1638
/* 794 */     MCD_OPC_CheckField, 21, 5, 0, 70, 3, // Skip to: 1638
/* 800 */     MCD_OPC_Decode, 131, 13, 46, // Opcode: SYNC_MM
/* 804 */     MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 816
/* 808 */     MCD_OPC_CheckPredicate, 3, 58, 3, // Skip to: 1638
/* 812 */     MCD_OPC_Decode, 133, 13, 41, // Opcode: SYSCALL_MM
/* 816 */     MCD_OPC_FilterValue, 13, 50, 3, // Skip to: 1638
/* 820 */     MCD_OPC_CheckPredicate, 3, 46, 3, // Skip to: 1638
/* 824 */     MCD_OPC_Decode, 154, 11, 41, // Opcode: SDBBP_MM
/* 828 */     MCD_OPC_FilterValue, 48, 8, 0, // Skip to: 840
/* 832 */     MCD_OPC_CheckPredicate, 3, 34, 3, // Skip to: 1638
/* 836 */     MCD_OPC_Decode, 212, 13, 40, // Opcode: TNE_MM
/* 840 */     MCD_OPC_FilterValue, 53, 75, 0, // Skip to: 919
/* 844 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 847 */     MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 865
/* 851 */     MCD_OPC_CheckPredicate, 3, 15, 3, // Skip to: 1638
/* 855 */     MCD_OPC_CheckField, 21, 5, 0, 9, 3, // Skip to: 1638
/* 861 */     MCD_OPC_Decode, 190, 8, 42, // Opcode: MFHI_MM
/* 865 */     MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 883
/* 869 */     MCD_OPC_CheckPredicate, 3, 253, 2, // Skip to: 1638
/* 873 */     MCD_OPC_CheckField, 21, 5, 0, 247, 2, // Skip to: 1638
/* 879 */     MCD_OPC_Decode, 195, 8, 42, // Opcode: MFLO_MM
/* 883 */     MCD_OPC_FilterValue, 2, 14, 0, // Skip to: 901
/* 887 */     MCD_OPC_CheckPredicate, 3, 235, 2, // Skip to: 1638
/* 891 */     MCD_OPC_CheckField, 21, 5, 0, 229, 2, // Skip to: 1638
/* 897 */     MCD_OPC_Decode, 179, 9, 42, // Opcode: MTHI_MM
/* 901 */     MCD_OPC_FilterValue, 3, 221, 2, // Skip to: 1638
/* 905 */     MCD_OPC_CheckPredicate, 3, 217, 2, // Skip to: 1638
/* 909 */     MCD_OPC_CheckField, 21, 5, 0, 211, 2, // Skip to: 1638
/* 915 */     MCD_OPC_Decode, 184, 9, 42, // Opcode: MTLO_MM
/* 919 */     MCD_OPC_FilterValue, 60, 203, 2, // Skip to: 1638
/* 923 */     MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 926 */     MCD_OPC_FilterValue, 0, 22, 0, // Skip to: 952
/* 930 */     MCD_OPC_CheckPredicate, 3, 10, 0, // Skip to: 944
/* 934 */     MCD_OPC_CheckField, 21, 5, 0, 4, 0, // Skip to: 944
/* 940 */     MCD_OPC_Decode, 143, 7, 42, // Opcode: JR_MM
/* 944 */     MCD_OPC_CheckPredicate, 3, 178, 2, // Skip to: 1638
/* 948 */     MCD_OPC_Decode, 129, 7, 43, // Opcode: JALR_MM
/* 952 */     MCD_OPC_FilterValue, 4, 170, 2, // Skip to: 1638
/* 956 */     MCD_OPC_CheckPredicate, 3, 166, 2, // Skip to: 1638
/* 960 */     MCD_OPC_Decode, 255, 6, 43, // Opcode: JALRS_MM
/* 964 */     MCD_OPC_FilterValue, 4, 7, 0, // Skip to: 975
/* 968 */     MCD_OPC_CheckPredicate, 3, 154, 2, // Skip to: 1638
/* 972 */     MCD_OPC_Decode, 74, 47, // Opcode: ADDi_MM
/* 975 */     MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 987
/* 979 */     MCD_OPC_CheckPredicate, 3, 143, 2, // Skip to: 1638
/* 983 */     MCD_OPC_Decode, 160, 7, 48, // Opcode: LBu_MM
/* 987 */     MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 999
/* 991 */     MCD_OPC_CheckPredicate, 3, 131, 2, // Skip to: 1638
/* 995 */     MCD_OPC_Decode, 145, 11, 48, // Opcode: SB_MM
/* 999 */     MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 1011
/* 1003 */    MCD_OPC_CheckPredicate, 3, 119, 2, // Skip to: 1638
/* 1007 */    MCD_OPC_Decode, 157, 7, 48, // Opcode: LB_MM
/* 1011 */    MCD_OPC_FilterValue, 8, 63, 0, // Skip to: 1078
/* 1015 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 1018 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1030
/* 1022 */    MCD_OPC_CheckPredicate, 3, 100, 2, // Skip to: 1638
/* 1026 */    MCD_OPC_Decode, 229, 7, 49, // Opcode: LWP_MM
/* 1030 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 1042
/* 1034 */    MCD_OPC_CheckPredicate, 3, 88, 2, // Skip to: 1638
/* 1038 */    MCD_OPC_Decode, 226, 7, 49, // Opcode: LWM32_MM
/* 1042 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 1054
/* 1046 */    MCD_OPC_CheckPredicate, 3, 76, 2, // Skip to: 1638
/* 1050 */    MCD_OPC_Decode, 221, 2, 50, // Opcode: CACHE_MM
/* 1054 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1066
/* 1058 */    MCD_OPC_CheckPredicate, 3, 64, 2, // Skip to: 1638
/* 1062 */    MCD_OPC_Decode, 249, 12, 49, // Opcode: SWP_MM
/* 1066 */    MCD_OPC_FilterValue, 13, 56, 2, // Skip to: 1638
/* 1070 */    MCD_OPC_CheckPredicate, 3, 52, 2, // Skip to: 1638
/* 1074 */    MCD_OPC_Decode, 247, 12, 49, // Opcode: SWM32_MM
/* 1078 */    MCD_OPC_FilterValue, 12, 7, 0, // Skip to: 1089
/* 1082 */    MCD_OPC_CheckPredicate, 3, 40, 2, // Skip to: 1638
/* 1086 */    MCD_OPC_Decode, 76, 47, // Opcode: ADDiu_MM
/* 1089 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 1101
/* 1093 */    MCD_OPC_CheckPredicate, 3, 29, 2, // Skip to: 1638
/* 1097 */    MCD_OPC_Decode, 191, 7, 48, // Opcode: LHu_MM
/* 1101 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1113
/* 1105 */    MCD_OPC_CheckPredicate, 3, 17, 2, // Skip to: 1638
/* 1109 */    MCD_OPC_Decode, 216, 11, 48, // Opcode: SH_MM
/* 1113 */    MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 1125
/* 1117 */    MCD_OPC_CheckPredicate, 3, 5, 2, // Skip to: 1638
/* 1121 */    MCD_OPC_Decode, 188, 7, 48, // Opcode: LH_MM
/* 1125 */    MCD_OPC_FilterValue, 16, 207, 0, // Skip to: 1336
/* 1129 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 1132 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1144
/* 1136 */    MCD_OPC_CheckPredicate, 3, 242, 1, // Skip to: 1638
/* 1140 */    MCD_OPC_Decode, 140, 2, 51, // Opcode: BLTZ_MM
/* 1144 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1156
/* 1148 */    MCD_OPC_CheckPredicate, 3, 230, 1, // Skip to: 1638
/* 1152 */    MCD_OPC_Decode, 137, 2, 51, // Opcode: BLTZAL_MM
/* 1156 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1168
/* 1160 */    MCD_OPC_CheckPredicate, 3, 218, 1, // Skip to: 1638
/* 1164 */    MCD_OPC_Decode, 226, 1, 51, // Opcode: BGEZ_MM
/* 1168 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1180
/* 1172 */    MCD_OPC_CheckPredicate, 3, 206, 1, // Skip to: 1638
/* 1176 */    MCD_OPC_Decode, 223, 1, 51, // Opcode: BGEZAL_MM
/* 1180 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 1192
/* 1184 */    MCD_OPC_CheckPredicate, 3, 194, 1, // Skip to: 1638
/* 1188 */    MCD_OPC_Decode, 128, 2, 51, // Opcode: BLEZ_MM
/* 1192 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 1204
/* 1196 */    MCD_OPC_CheckPredicate, 3, 182, 1, // Skip to: 1638
/* 1200 */    MCD_OPC_Decode, 160, 2, 51, // Opcode: BNEZC_MM
/* 1204 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 1216
/* 1208 */    MCD_OPC_CheckPredicate, 3, 170, 1, // Skip to: 1638
/* 1212 */    MCD_OPC_Decode, 232, 1, 51, // Opcode: BGTZ_MM
/* 1216 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 1228
/* 1220 */    MCD_OPC_CheckPredicate, 3, 158, 1, // Skip to: 1638
/* 1224 */    MCD_OPC_Decode, 213, 1, 51, // Opcode: BEQZC_MM
/* 1228 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1240
/* 1232 */    MCD_OPC_CheckPredicate, 3, 146, 1, // Skip to: 1638
/* 1236 */    MCD_OPC_Decode, 205, 13, 52, // Opcode: TLTI_MM
/* 1240 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1252
/* 1244 */    MCD_OPC_CheckPredicate, 3, 134, 1, // Skip to: 1638
/* 1248 */    MCD_OPC_Decode, 190, 13, 52, // Opcode: TGEI_MM
/* 1252 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 1264
/* 1256 */    MCD_OPC_CheckPredicate, 3, 122, 1, // Skip to: 1638
/* 1260 */    MCD_OPC_Decode, 204, 13, 52, // Opcode: TLTIU_MM
/* 1264 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1276
/* 1268 */    MCD_OPC_CheckPredicate, 3, 110, 1, // Skip to: 1638
/* 1272 */    MCD_OPC_Decode, 189, 13, 52, // Opcode: TGEIU_MM
/* 1276 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 1288
/* 1280 */    MCD_OPC_CheckPredicate, 3, 98, 1, // Skip to: 1638
/* 1284 */    MCD_OPC_Decode, 211, 13, 52, // Opcode: TNEI_MM
/* 1288 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 1300
/* 1292 */    MCD_OPC_CheckPredicate, 3, 86, 1, // Skip to: 1638
/* 1296 */    MCD_OPC_Decode, 212, 7, 52, // Opcode: LUi_MM
/* 1300 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1312
/* 1304 */    MCD_OPC_CheckPredicate, 3, 74, 1, // Skip to: 1638
/* 1308 */    MCD_OPC_Decode, 184, 13, 52, // Opcode: TEQI_MM
/* 1312 */    MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 1324
/* 1316 */    MCD_OPC_CheckPredicate, 3, 62, 1, // Skip to: 1638
/* 1320 */    MCD_OPC_Decode, 136, 2, 51, // Opcode: BLTZALS_MM
/* 1324 */    MCD_OPC_FilterValue, 19, 54, 1, // Skip to: 1638
/* 1328 */    MCD_OPC_CheckPredicate, 3, 50, 1, // Skip to: 1638
/* 1332 */    MCD_OPC_Decode, 222, 1, 51, // Opcode: BGEZALS_MM
/* 1336 */    MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 1348
/* 1340 */    MCD_OPC_CheckPredicate, 3, 38, 1, // Skip to: 1638
/* 1344 */    MCD_OPC_Decode, 144, 10, 53, // Opcode: ORi_MM
/* 1348 */    MCD_OPC_FilterValue, 21, 29, 0, // Skip to: 1381
/* 1352 */    MCD_OPC_ExtractField, 0, 13,  // Inst{12-0} ...
/* 1355 */    MCD_OPC_FilterValue, 251, 2, 8, 0, // Skip to: 1368
/* 1360 */    MCD_OPC_CheckPredicate, 3, 18, 1, // Skip to: 1638
/* 1364 */    MCD_OPC_Decode, 241, 8, 54, // Opcode: MOVF_I_MM
/* 1368 */    MCD_OPC_FilterValue, 251, 18, 9, 1, // Skip to: 1638
/* 1373 */    MCD_OPC_CheckPredicate, 3, 5, 1, // Skip to: 1638
/* 1377 */    MCD_OPC_Decode, 133, 9, 54, // Opcode: MOVT_I_MM
/* 1381 */    MCD_OPC_FilterValue, 24, 99, 0, // Skip to: 1484
/* 1385 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 1388 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1400
/* 1392 */    MCD_OPC_CheckPredicate, 3, 242, 0, // Skip to: 1638
/* 1396 */    MCD_OPC_Decode, 224, 7, 49, // Opcode: LWL_MM
/* 1400 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1412
/* 1404 */    MCD_OPC_CheckPredicate, 3, 230, 0, // Skip to: 1638
/* 1408 */    MCD_OPC_Decode, 232, 7, 49, // Opcode: LWR_MM
/* 1412 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1424
/* 1416 */    MCD_OPC_CheckPredicate, 3, 218, 0, // Skip to: 1638
/* 1420 */    MCD_OPC_Decode, 182, 10, 50, // Opcode: PREF_MM
/* 1424 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1436
/* 1428 */    MCD_OPC_CheckPredicate, 3, 206, 0, // Skip to: 1638
/* 1432 */    MCD_OPC_Decode, 196, 7, 49, // Opcode: LL_MM
/* 1436 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1448
/* 1440 */    MCD_OPC_CheckPredicate, 3, 194, 0, // Skip to: 1638
/* 1444 */    MCD_OPC_Decode, 245, 12, 49, // Opcode: SWL_MM
/* 1448 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1460
/* 1452 */    MCD_OPC_CheckPredicate, 3, 182, 0, // Skip to: 1638
/* 1456 */    MCD_OPC_Decode, 252, 12, 49, // Opcode: SWR_MM
/* 1460 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1472
/* 1464 */    MCD_OPC_CheckPredicate, 3, 170, 0, // Skip to: 1638
/* 1468 */    MCD_OPC_Decode, 149, 11, 49, // Opcode: SC_MM
/* 1472 */    MCD_OPC_FilterValue, 14, 162, 0, // Skip to: 1638
/* 1476 */    MCD_OPC_CheckPredicate, 3, 158, 0, // Skip to: 1638
/* 1480 */    MCD_OPC_Decode, 235, 7, 49, // Opcode: LWU_MM
/* 1484 */    MCD_OPC_FilterValue, 28, 8, 0, // Skip to: 1496
/* 1488 */    MCD_OPC_CheckPredicate, 3, 146, 0, // Skip to: 1638
/* 1492 */    MCD_OPC_Decode, 247, 13, 53, // Opcode: XORi_MM
/* 1496 */    MCD_OPC_FilterValue, 29, 8, 0, // Skip to: 1508
/* 1500 */    MCD_OPC_CheckPredicate, 3, 134, 0, // Skip to: 1638
/* 1504 */    MCD_OPC_Decode, 130, 7, 55, // Opcode: JALS_MM
/* 1508 */    MCD_OPC_FilterValue, 30, 7, 0, // Skip to: 1519
/* 1512 */    MCD_OPC_CheckPredicate, 3, 122, 0, // Skip to: 1638
/* 1516 */    MCD_OPC_Decode, 27, 56, // Opcode: ADDIUPC_MM
/* 1519 */    MCD_OPC_FilterValue, 36, 8, 0, // Skip to: 1531
/* 1523 */    MCD_OPC_CheckPredicate, 3, 111, 0, // Skip to: 1638
/* 1527 */    MCD_OPC_Decode, 245, 11, 47, // Opcode: SLTi_MM
/* 1531 */    MCD_OPC_FilterValue, 37, 8, 0, // Skip to: 1543
/* 1535 */    MCD_OPC_CheckPredicate, 3, 99, 0, // Skip to: 1638
/* 1539 */    MCD_OPC_Decode, 214, 1, 57, // Opcode: BEQ_MM
/* 1543 */    MCD_OPC_FilterValue, 44, 8, 0, // Skip to: 1555
/* 1547 */    MCD_OPC_CheckPredicate, 3, 87, 0, // Skip to: 1638
/* 1551 */    MCD_OPC_Decode, 248, 11, 47, // Opcode: SLTiu_MM
/* 1555 */    MCD_OPC_FilterValue, 45, 8, 0, // Skip to: 1567
/* 1559 */    MCD_OPC_CheckPredicate, 3, 75, 0, // Skip to: 1638
/* 1563 */    MCD_OPC_Decode, 161, 2, 57, // Opcode: BNE_MM
/* 1567 */    MCD_OPC_FilterValue, 52, 7, 0, // Skip to: 1578
/* 1571 */    MCD_OPC_CheckPredicate, 3, 63, 0, // Skip to: 1638
/* 1575 */    MCD_OPC_Decode, 95, 53, // Opcode: ANDi_MM
/* 1578 */    MCD_OPC_FilterValue, 53, 8, 0, // Skip to: 1590
/* 1582 */    MCD_OPC_CheckPredicate, 3, 52, 0, // Skip to: 1638
/* 1586 */    MCD_OPC_Decode, 144, 7, 55, // Opcode: J_MM
/* 1590 */    MCD_OPC_FilterValue, 60, 8, 0, // Skip to: 1602
/* 1594 */    MCD_OPC_CheckPredicate, 3, 40, 0, // Skip to: 1638
/* 1598 */    MCD_OPC_Decode, 132, 7, 55, // Opcode: JALX_MM
/* 1602 */    MCD_OPC_FilterValue, 61, 8, 0, // Skip to: 1614
/* 1606 */    MCD_OPC_CheckPredicate, 3, 28, 0, // Skip to: 1638
/* 1610 */    MCD_OPC_Decode, 133, 7, 55, // Opcode: JAL_MM
/* 1614 */    MCD_OPC_FilterValue, 62, 8, 0, // Skip to: 1626
/* 1618 */    MCD_OPC_CheckPredicate, 3, 16, 0, // Skip to: 1638
/* 1622 */    MCD_OPC_Decode, 128, 13, 48, // Opcode: SW_MM
/* 1626 */    MCD_OPC_FilterValue, 63, 8, 0, // Skip to: 1638
/* 1630 */    MCD_OPC_CheckPredicate, 3, 4, 0, // Skip to: 1638
/* 1634 */    MCD_OPC_Decode, 240, 7, 48, // Opcode: LW_MM
/* 1638 */    MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTableMips32[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 0, 173, 3, // Skip to: 948
/* 7 */       MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 10 */      MCD_OPC_FilterValue, 0, 54, 0, // Skip to: 68
/* 14 */      MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 17 */      MCD_OPC_FilterValue, 0, 137, 53, // Skip to: 13726
/* 21 */      MCD_OPC_ExtractField, 6, 15,  // Inst{20-6} ...
/* 24 */      MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 36
/* 28 */      MCD_OPC_CheckPredicate, 5, 28, 0, // Skip to: 60
/* 32 */      MCD_OPC_Decode, 180, 12, 0, // Opcode: SSNOP
/* 36 */      MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 48
/* 40 */      MCD_OPC_CheckPredicate, 5, 16, 0, // Skip to: 60
/* 44 */      MCD_OPC_Decode, 139, 5, 0, // Opcode: EHB
/* 48 */      MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 60
/* 52 */      MCD_OPC_CheckPredicate, 6, 4, 0, // Skip to: 60
/* 56 */      MCD_OPC_Decode, 147, 10, 0, // Opcode: PAUSE
/* 60 */      MCD_OPC_CheckPredicate, 1, 94, 53, // Skip to: 13726
/* 64 */      MCD_OPC_Decode, 225, 11, 58, // Opcode: SLL
/* 68 */      MCD_OPC_FilterValue, 1, 39, 0, // Skip to: 111
/* 72 */      MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 75 */      MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 93
/* 79 */      MCD_OPC_CheckPredicate, 7, 75, 53, // Skip to: 13726
/* 83 */      MCD_OPC_CheckField, 6, 5, 0, 69, 53, // Skip to: 13726
/* 89 */      MCD_OPC_Decode, 239, 8, 59, // Opcode: MOVF_I
/* 93 */      MCD_OPC_FilterValue, 1, 61, 53, // Skip to: 13726
/* 97 */      MCD_OPC_CheckPredicate, 7, 57, 53, // Skip to: 13726
/* 101 */     MCD_OPC_CheckField, 6, 5, 0, 51, 53, // Skip to: 13726
/* 107 */     MCD_OPC_Decode, 131, 9, 59, // Opcode: MOVT_I
/* 111 */     MCD_OPC_FilterValue, 2, 27, 0, // Skip to: 142
/* 115 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 118 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 130
/* 122 */     MCD_OPC_CheckPredicate, 1, 32, 53, // Skip to: 13726
/* 126 */     MCD_OPC_Decode, 159, 12, 58, // Opcode: SRL
/* 130 */     MCD_OPC_FilterValue, 1, 24, 53, // Skip to: 13726
/* 134 */     MCD_OPC_CheckPredicate, 6, 20, 53, // Skip to: 13726
/* 138 */     MCD_OPC_Decode, 247, 10, 58, // Opcode: ROTR
/* 142 */     MCD_OPC_FilterValue, 3, 14, 0, // Skip to: 160
/* 146 */     MCD_OPC_CheckPredicate, 5, 8, 53, // Skip to: 13726
/* 150 */     MCD_OPC_CheckField, 21, 5, 0, 2, 53, // Skip to: 13726
/* 156 */     MCD_OPC_Decode, 139, 12, 58, // Opcode: SRA
/* 160 */     MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 178
/* 164 */     MCD_OPC_CheckPredicate, 5, 246, 52, // Skip to: 13726
/* 168 */     MCD_OPC_CheckField, 6, 5, 0, 240, 52, // Skip to: 13726
/* 174 */     MCD_OPC_Decode, 233, 11, 36, // Opcode: SLLV
/* 178 */     MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 196
/* 182 */     MCD_OPC_CheckPredicate, 8, 228, 52, // Skip to: 13726
/* 186 */     MCD_OPC_CheckField, 8, 3, 0, 222, 52, // Skip to: 13726
/* 192 */     MCD_OPC_Decode, 205, 7, 60, // Opcode: LSA
/* 196 */     MCD_OPC_FilterValue, 6, 27, 0, // Skip to: 227
/* 200 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 203 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 215
/* 207 */     MCD_OPC_CheckPredicate, 5, 203, 52, // Skip to: 13726
/* 211 */     MCD_OPC_Decode, 173, 12, 36, // Opcode: SRLV
/* 215 */     MCD_OPC_FilterValue, 1, 195, 52, // Skip to: 13726
/* 219 */     MCD_OPC_CheckPredicate, 6, 191, 52, // Skip to: 13726
/* 223 */     MCD_OPC_Decode, 248, 10, 36, // Opcode: ROTRV
/* 227 */     MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 245
/* 231 */     MCD_OPC_CheckPredicate, 5, 179, 52, // Skip to: 13726
/* 235 */     MCD_OPC_CheckField, 6, 5, 0, 173, 52, // Skip to: 13726
/* 241 */     MCD_OPC_Decode, 152, 12, 36, // Opcode: SRAV
/* 245 */     MCD_OPC_FilterValue, 8, 27, 0, // Skip to: 276
/* 249 */     MCD_OPC_ExtractField, 6, 15,  // Inst{20-6} ...
/* 252 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 264
/* 256 */     MCD_OPC_CheckPredicate, 5, 154, 52, // Skip to: 13726
/* 260 */     MCD_OPC_Decode, 136, 7, 61, // Opcode: JR
/* 264 */     MCD_OPC_FilterValue, 16, 146, 52, // Skip to: 13726
/* 268 */     MCD_OPC_CheckPredicate, 9, 142, 52, // Skip to: 13726
/* 272 */     MCD_OPC_Decode, 141, 7, 61, // Opcode: JR_HB
/* 276 */     MCD_OPC_FilterValue, 9, 39, 0, // Skip to: 319
/* 280 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 283 */     MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 301
/* 287 */     MCD_OPC_CheckPredicate, 1, 123, 52, // Skip to: 13726
/* 291 */     MCD_OPC_CheckField, 16, 5, 0, 117, 52, // Skip to: 13726
/* 297 */     MCD_OPC_Decode, 249, 6, 62, // Opcode: JALR
/* 301 */     MCD_OPC_FilterValue, 16, 109, 52, // Skip to: 13726
/* 305 */     MCD_OPC_CheckPredicate, 10, 105, 52, // Skip to: 13726
/* 309 */     MCD_OPC_CheckField, 16, 5, 0, 99, 52, // Skip to: 13726
/* 315 */     MCD_OPC_Decode, 128, 7, 62, // Opcode: JALR_HB
/* 319 */     MCD_OPC_FilterValue, 10, 14, 0, // Skip to: 337
/* 323 */     MCD_OPC_CheckPredicate, 7, 87, 52, // Skip to: 13726
/* 327 */     MCD_OPC_CheckField, 6, 5, 0, 81, 52, // Skip to: 13726
/* 333 */     MCD_OPC_Decode, 143, 9, 63, // Opcode: MOVZ_I_I
/* 337 */     MCD_OPC_FilterValue, 11, 14, 0, // Skip to: 355
/* 341 */     MCD_OPC_CheckPredicate, 7, 69, 52, // Skip to: 13726
/* 345 */     MCD_OPC_CheckField, 6, 5, 0, 63, 52, // Skip to: 13726
/* 351 */     MCD_OPC_Decode, 251, 8, 63, // Opcode: MOVN_I_I
/* 355 */     MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 367
/* 359 */     MCD_OPC_CheckPredicate, 5, 51, 52, // Skip to: 13726
/* 363 */     MCD_OPC_Decode, 132, 13, 64, // Opcode: SYSCALL
/* 367 */     MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 379
/* 371 */     MCD_OPC_CheckPredicate, 5, 39, 52, // Skip to: 13726
/* 375 */     MCD_OPC_Decode, 171, 2, 33, // Opcode: BREAK
/* 379 */     MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 391
/* 383 */     MCD_OPC_CheckPredicate, 10, 27, 52, // Skip to: 13726
/* 387 */     MCD_OPC_Decode, 129, 13, 65, // Opcode: SYNC
/* 391 */     MCD_OPC_FilterValue, 16, 43, 0, // Skip to: 438
/* 395 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 398 */     MCD_OPC_FilterValue, 0, 12, 52, // Skip to: 13726
/* 402 */     MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 405 */     MCD_OPC_FilterValue, 0, 5, 52, // Skip to: 13726
/* 409 */     MCD_OPC_ExtractField, 23, 3,  // Inst{25-23} ...
/* 412 */     MCD_OPC_FilterValue, 0, 254, 51, // Skip to: 13726
/* 416 */     MCD_OPC_CheckPredicate, 11, 10, 0, // Skip to: 430
/* 420 */     MCD_OPC_CheckField, 21, 2, 0, 4, 0, // Skip to: 430
/* 426 */     MCD_OPC_Decode, 186, 8, 66, // Opcode: MFHI
/* 430 */     MCD_OPC_CheckPredicate, 12, 236, 51, // Skip to: 13726
/* 434 */     MCD_OPC_Decode, 189, 8, 67, // Opcode: MFHI_DSP
/* 438 */     MCD_OPC_FilterValue, 17, 36, 0, // Skip to: 478
/* 442 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 445 */     MCD_OPC_FilterValue, 0, 221, 51, // Skip to: 13726
/* 449 */     MCD_OPC_ExtractField, 13, 8,  // Inst{20-13} ...
/* 452 */     MCD_OPC_FilterValue, 0, 214, 51, // Skip to: 13726
/* 456 */     MCD_OPC_CheckPredicate, 13, 10, 0, // Skip to: 470
/* 460 */     MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 470
/* 466 */     MCD_OPC_Decode, 176, 9, 61, // Opcode: MTHI
/* 470 */     MCD_OPC_CheckPredicate, 12, 196, 51, // Skip to: 13726
/* 474 */     MCD_OPC_Decode, 178, 9, 68, // Opcode: MTHI_DSP
/* 478 */     MCD_OPC_FilterValue, 18, 43, 0, // Skip to: 525
/* 482 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 485 */     MCD_OPC_FilterValue, 0, 181, 51, // Skip to: 13726
/* 489 */     MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 492 */     MCD_OPC_FilterValue, 0, 174, 51, // Skip to: 13726
/* 496 */     MCD_OPC_ExtractField, 23, 3,  // Inst{25-23} ...
/* 499 */     MCD_OPC_FilterValue, 0, 167, 51, // Skip to: 13726
/* 503 */     MCD_OPC_CheckPredicate, 11, 10, 0, // Skip to: 517
/* 507 */     MCD_OPC_CheckField, 21, 2, 0, 4, 0, // Skip to: 517
/* 513 */     MCD_OPC_Decode, 191, 8, 66, // Opcode: MFLO
/* 517 */     MCD_OPC_CheckPredicate, 12, 149, 51, // Skip to: 13726
/* 521 */     MCD_OPC_Decode, 194, 8, 67, // Opcode: MFLO_DSP
/* 525 */     MCD_OPC_FilterValue, 19, 36, 0, // Skip to: 565
/* 529 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 532 */     MCD_OPC_FilterValue, 0, 134, 51, // Skip to: 13726
/* 536 */     MCD_OPC_ExtractField, 13, 8,  // Inst{20-13} ...
/* 539 */     MCD_OPC_FilterValue, 0, 127, 51, // Skip to: 13726
/* 543 */     MCD_OPC_CheckPredicate, 13, 10, 0, // Skip to: 557
/* 547 */     MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 557
/* 553 */     MCD_OPC_Decode, 181, 9, 61, // Opcode: MTLO
/* 557 */     MCD_OPC_CheckPredicate, 12, 109, 51, // Skip to: 13726
/* 561 */     MCD_OPC_Decode, 183, 9, 69, // Opcode: MTLO_DSP
/* 565 */     MCD_OPC_FilterValue, 21, 14, 0, // Skip to: 583
/* 569 */     MCD_OPC_CheckPredicate, 14, 97, 51, // Skip to: 13726
/* 573 */     MCD_OPC_CheckField, 8, 3, 0, 91, 51, // Skip to: 13726
/* 579 */     MCD_OPC_Decode, 194, 4, 70, // Opcode: DLSA
/* 583 */     MCD_OPC_FilterValue, 24, 36, 0, // Skip to: 623
/* 587 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 590 */     MCD_OPC_FilterValue, 0, 76, 51, // Skip to: 13726
/* 594 */     MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 597 */     MCD_OPC_FilterValue, 0, 69, 51, // Skip to: 13726
/* 601 */     MCD_OPC_CheckPredicate, 13, 10, 0, // Skip to: 615
/* 605 */     MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 615
/* 611 */     MCD_OPC_Decode, 206, 9, 43, // Opcode: MULT
/* 615 */     MCD_OPC_CheckPredicate, 12, 51, 51, // Skip to: 13726
/* 619 */     MCD_OPC_Decode, 208, 9, 71, // Opcode: MULT_DSP
/* 623 */     MCD_OPC_FilterValue, 25, 36, 0, // Skip to: 663
/* 627 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 630 */     MCD_OPC_FilterValue, 0, 36, 51, // Skip to: 13726
/* 634 */     MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 637 */     MCD_OPC_FilterValue, 0, 29, 51, // Skip to: 13726
/* 641 */     MCD_OPC_CheckPredicate, 13, 10, 0, // Skip to: 655
/* 645 */     MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 655
/* 651 */     MCD_OPC_Decode, 210, 9, 43, // Opcode: MULTu
/* 655 */     MCD_OPC_CheckPredicate, 12, 11, 51, // Skip to: 13726
/* 659 */     MCD_OPC_Decode, 207, 9, 71, // Opcode: MULTU_DSP
/* 663 */     MCD_OPC_FilterValue, 26, 14, 0, // Skip to: 681
/* 667 */     MCD_OPC_CheckPredicate, 13, 255, 50, // Skip to: 13726
/* 671 */     MCD_OPC_CheckField, 6, 10, 0, 249, 50, // Skip to: 13726
/* 677 */     MCD_OPC_Decode, 162, 11, 43, // Opcode: SDIV
/* 681 */     MCD_OPC_FilterValue, 27, 14, 0, // Skip to: 699
/* 685 */     MCD_OPC_CheckPredicate, 13, 237, 50, // Skip to: 13726
/* 689 */     MCD_OPC_CheckField, 6, 10, 0, 231, 50, // Skip to: 13726
/* 695 */     MCD_OPC_Decode, 222, 13, 43, // Opcode: UDIV
/* 699 */     MCD_OPC_FilterValue, 32, 13, 0, // Skip to: 716
/* 703 */     MCD_OPC_CheckPredicate, 5, 219, 50, // Skip to: 13726
/* 707 */     MCD_OPC_CheckField, 6, 5, 0, 213, 50, // Skip to: 13726
/* 713 */     MCD_OPC_Decode, 25, 35, // Opcode: ADD
/* 716 */     MCD_OPC_FilterValue, 33, 13, 0, // Skip to: 733
/* 720 */     MCD_OPC_CheckPredicate, 5, 202, 50, // Skip to: 13726
/* 724 */     MCD_OPC_CheckField, 6, 5, 0, 196, 50, // Skip to: 13726
/* 730 */     MCD_OPC_Decode, 77, 35, // Opcode: ADDu
/* 733 */     MCD_OPC_FilterValue, 34, 14, 0, // Skip to: 751
/* 737 */     MCD_OPC_CheckPredicate, 5, 185, 50, // Skip to: 13726
/* 741 */     MCD_OPC_CheckField, 6, 5, 0, 179, 50, // Skip to: 13726
/* 747 */     MCD_OPC_Decode, 190, 12, 35, // Opcode: SUB
/* 751 */     MCD_OPC_FilterValue, 35, 14, 0, // Skip to: 769
/* 755 */     MCD_OPC_CheckPredicate, 5, 167, 50, // Skip to: 13726
/* 759 */     MCD_OPC_CheckField, 6, 5, 0, 161, 50, // Skip to: 13726
/* 765 */     MCD_OPC_Decode, 230, 12, 35, // Opcode: SUBu
/* 769 */     MCD_OPC_FilterValue, 36, 13, 0, // Skip to: 786
/* 773 */     MCD_OPC_CheckPredicate, 1, 149, 50, // Skip to: 13726
/* 777 */     MCD_OPC_CheckField, 6, 5, 0, 143, 50, // Skip to: 13726
/* 783 */     MCD_OPC_Decode, 83, 35, // Opcode: AND
/* 786 */     MCD_OPC_FilterValue, 37, 14, 0, // Skip to: 804
/* 790 */     MCD_OPC_CheckPredicate, 1, 132, 50, // Skip to: 13726
/* 794 */     MCD_OPC_CheckField, 6, 5, 0, 126, 50, // Skip to: 13726
/* 800 */     MCD_OPC_Decode, 133, 10, 35, // Opcode: OR
/* 804 */     MCD_OPC_FilterValue, 38, 14, 0, // Skip to: 822
/* 808 */     MCD_OPC_CheckPredicate, 1, 114, 50, // Skip to: 13726
/* 812 */     MCD_OPC_CheckField, 6, 5, 0, 108, 50, // Skip to: 13726
/* 818 */     MCD_OPC_Decode, 236, 13, 35, // Opcode: XOR
/* 822 */     MCD_OPC_FilterValue, 39, 14, 0, // Skip to: 840
/* 826 */     MCD_OPC_CheckPredicate, 5, 96, 50, // Skip to: 13726
/* 830 */     MCD_OPC_CheckField, 6, 5, 0, 90, 50, // Skip to: 13726
/* 836 */     MCD_OPC_Decode, 250, 9, 35, // Opcode: NOR
/* 840 */     MCD_OPC_FilterValue, 42, 14, 0, // Skip to: 858
/* 844 */     MCD_OPC_CheckPredicate, 5, 78, 50, // Skip to: 13726
/* 848 */     MCD_OPC_CheckField, 6, 5, 0, 72, 50, // Skip to: 13726
/* 854 */     MCD_OPC_Decode, 240, 11, 35, // Opcode: SLT
/* 858 */     MCD_OPC_FilterValue, 43, 14, 0, // Skip to: 876
/* 862 */     MCD_OPC_CheckPredicate, 5, 60, 50, // Skip to: 13726
/* 866 */     MCD_OPC_CheckField, 6, 5, 0, 54, 50, // Skip to: 13726
/* 872 */     MCD_OPC_Decode, 249, 11, 35, // Opcode: SLTu
/* 876 */     MCD_OPC_FilterValue, 48, 8, 0, // Skip to: 888
/* 880 */     MCD_OPC_CheckPredicate, 15, 42, 50, // Skip to: 13726
/* 884 */     MCD_OPC_Decode, 186, 13, 72, // Opcode: TGE
/* 888 */     MCD_OPC_FilterValue, 49, 8, 0, // Skip to: 900
/* 892 */     MCD_OPC_CheckPredicate, 15, 30, 50, // Skip to: 13726
/* 896 */     MCD_OPC_Decode, 191, 13, 72, // Opcode: TGEU
/* 900 */     MCD_OPC_FilterValue, 50, 8, 0, // Skip to: 912
/* 904 */     MCD_OPC_CheckPredicate, 15, 18, 50, // Skip to: 13726
/* 908 */     MCD_OPC_Decode, 202, 13, 72, // Opcode: TLT
/* 912 */     MCD_OPC_FilterValue, 51, 8, 0, // Skip to: 924
/* 916 */     MCD_OPC_CheckPredicate, 15, 6, 50, // Skip to: 13726
/* 920 */     MCD_OPC_Decode, 206, 13, 72, // Opcode: TLTU
/* 924 */     MCD_OPC_FilterValue, 52, 8, 0, // Skip to: 936
/* 928 */     MCD_OPC_CheckPredicate, 15, 250, 49, // Skip to: 13726
/* 932 */     MCD_OPC_Decode, 182, 13, 72, // Opcode: TEQ
/* 936 */     MCD_OPC_FilterValue, 54, 242, 49, // Skip to: 13726
/* 940 */     MCD_OPC_CheckPredicate, 15, 238, 49, // Skip to: 13726
/* 944 */     MCD_OPC_Decode, 209, 13, 72, // Opcode: TNE
/* 948 */     MCD_OPC_FilterValue, 1, 201, 0, // Skip to: 1153
/* 952 */     MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 955 */     MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 967
/* 959 */     MCD_OPC_CheckPredicate, 5, 219, 49, // Skip to: 13726
/* 963 */     MCD_OPC_Decode, 131, 2, 73, // Opcode: BLTZ
/* 967 */     MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 979
/* 971 */     MCD_OPC_CheckPredicate, 5, 207, 49, // Skip to: 13726
/* 975 */     MCD_OPC_Decode, 217, 1, 73, // Opcode: BGEZ
/* 979 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 991
/* 983 */     MCD_OPC_CheckPredicate, 16, 195, 49, // Skip to: 13726
/* 987 */     MCD_OPC_Decode, 139, 2, 73, // Opcode: BLTZL
/* 991 */     MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1003
/* 995 */     MCD_OPC_CheckPredicate, 16, 183, 49, // Skip to: 13726
/* 999 */     MCD_OPC_Decode, 225, 1, 73, // Opcode: BGEZL
/* 1003 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1015
/* 1007 */    MCD_OPC_CheckPredicate, 16, 171, 49, // Skip to: 13726
/* 1011 */    MCD_OPC_Decode, 187, 13, 74, // Opcode: TGEI
/* 1015 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1027
/* 1019 */    MCD_OPC_CheckPredicate, 16, 159, 49, // Skip to: 13726
/* 1023 */    MCD_OPC_Decode, 188, 13, 74, // Opcode: TGEIU
/* 1027 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 1039
/* 1031 */    MCD_OPC_CheckPredicate, 16, 147, 49, // Skip to: 13726
/* 1035 */    MCD_OPC_Decode, 203, 13, 74, // Opcode: TLTI
/* 1039 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1051
/* 1043 */    MCD_OPC_CheckPredicate, 16, 135, 49, // Skip to: 13726
/* 1047 */    MCD_OPC_Decode, 221, 13, 74, // Opcode: TTLTIU
/* 1051 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 1063
/* 1055 */    MCD_OPC_CheckPredicate, 16, 123, 49, // Skip to: 13726
/* 1059 */    MCD_OPC_Decode, 183, 13, 74, // Opcode: TEQI
/* 1063 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1075
/* 1067 */    MCD_OPC_CheckPredicate, 16, 111, 49, // Skip to: 13726
/* 1071 */    MCD_OPC_Decode, 210, 13, 74, // Opcode: TNEI
/* 1075 */    MCD_OPC_FilterValue, 16, 8, 0, // Skip to: 1087
/* 1079 */    MCD_OPC_CheckPredicate, 13, 99, 49, // Skip to: 13726
/* 1083 */    MCD_OPC_Decode, 133, 2, 73, // Opcode: BLTZAL
/* 1087 */    MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 1099
/* 1091 */    MCD_OPC_CheckPredicate, 13, 87, 49, // Skip to: 13726
/* 1095 */    MCD_OPC_Decode, 219, 1, 73, // Opcode: BGEZAL
/* 1099 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 1111
/* 1103 */    MCD_OPC_CheckPredicate, 16, 75, 49, // Skip to: 13726
/* 1107 */    MCD_OPC_Decode, 135, 2, 73, // Opcode: BLTZALL
/* 1111 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 1123
/* 1115 */    MCD_OPC_CheckPredicate, 16, 63, 49, // Skip to: 13726
/* 1119 */    MCD_OPC_Decode, 221, 1, 73, // Opcode: BGEZALL
/* 1123 */    MCD_OPC_FilterValue, 28, 14, 0, // Skip to: 1141
/* 1127 */    MCD_OPC_CheckPredicate, 12, 51, 49, // Skip to: 13726
/* 1131 */    MCD_OPC_CheckField, 21, 5, 0, 45, 49, // Skip to: 13726
/* 1137 */    MCD_OPC_Decode, 169, 2, 75, // Opcode: BPOSGE32
/* 1141 */    MCD_OPC_FilterValue, 31, 37, 49, // Skip to: 13726
/* 1145 */    MCD_OPC_CheckPredicate, 6, 33, 49, // Skip to: 13726
/* 1149 */    MCD_OPC_Decode, 130, 13, 76, // Opcode: SYNCI
/* 1153 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1165
/* 1157 */    MCD_OPC_CheckPredicate, 10, 21, 49, // Skip to: 13726
/* 1161 */    MCD_OPC_Decode, 247, 6, 77, // Opcode: J
/* 1165 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1177
/* 1169 */    MCD_OPC_CheckPredicate, 5, 9, 49, // Skip to: 13726
/* 1173 */    MCD_OPC_Decode, 248, 6, 77, // Opcode: JAL
/* 1177 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 1189
/* 1181 */    MCD_OPC_CheckPredicate, 5, 253, 48, // Skip to: 13726
/* 1185 */    MCD_OPC_Decode, 206, 1, 78, // Opcode: BEQ
/* 1189 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 1201
/* 1193 */    MCD_OPC_CheckPredicate, 5, 241, 48, // Skip to: 13726
/* 1197 */    MCD_OPC_Decode, 145, 2, 78, // Opcode: BNE
/* 1201 */    MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 1219
/* 1205 */    MCD_OPC_CheckPredicate, 5, 229, 48, // Skip to: 13726
/* 1209 */    MCD_OPC_CheckField, 16, 5, 0, 223, 48, // Skip to: 13726
/* 1215 */    MCD_OPC_Decode, 251, 1, 73, // Opcode: BLEZ
/* 1219 */    MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 1237
/* 1223 */    MCD_OPC_CheckPredicate, 5, 211, 48, // Skip to: 13726
/* 1227 */    MCD_OPC_CheckField, 16, 5, 0, 205, 48, // Skip to: 13726
/* 1233 */    MCD_OPC_Decode, 227, 1, 73, // Opcode: BGTZ
/* 1237 */    MCD_OPC_FilterValue, 8, 7, 0, // Skip to: 1248
/* 1241 */    MCD_OPC_CheckPredicate, 13, 193, 48, // Skip to: 13726
/* 1245 */    MCD_OPC_Decode, 73, 79, // Opcode: ADDi
/* 1248 */    MCD_OPC_FilterValue, 9, 7, 0, // Skip to: 1259
/* 1252 */    MCD_OPC_CheckPredicate, 1, 182, 48, // Skip to: 13726
/* 1256 */    MCD_OPC_Decode, 75, 79, // Opcode: ADDiu
/* 1259 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 1271
/* 1263 */    MCD_OPC_CheckPredicate, 5, 171, 48, // Skip to: 13726
/* 1267 */    MCD_OPC_Decode, 243, 11, 79, // Opcode: SLTi
/* 1271 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1283
/* 1275 */    MCD_OPC_CheckPredicate, 5, 159, 48, // Skip to: 13726
/* 1279 */    MCD_OPC_Decode, 246, 11, 79, // Opcode: SLTiu
/* 1283 */    MCD_OPC_FilterValue, 12, 7, 0, // Skip to: 1294
/* 1287 */    MCD_OPC_CheckPredicate, 1, 147, 48, // Skip to: 13726
/* 1291 */    MCD_OPC_Decode, 93, 80, // Opcode: ANDi
/* 1294 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 1306
/* 1298 */    MCD_OPC_CheckPredicate, 5, 136, 48, // Skip to: 13726
/* 1302 */    MCD_OPC_Decode, 142, 10, 80, // Opcode: ORi
/* 1306 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1318
/* 1310 */    MCD_OPC_CheckPredicate, 5, 124, 48, // Skip to: 13726
/* 1314 */    MCD_OPC_Decode, 245, 13, 80, // Opcode: XORi
/* 1318 */    MCD_OPC_FilterValue, 15, 14, 0, // Skip to: 1336
/* 1322 */    MCD_OPC_CheckPredicate, 5, 112, 48, // Skip to: 13726
/* 1326 */    MCD_OPC_CheckField, 21, 5, 0, 106, 48, // Skip to: 13726
/* 1332 */    MCD_OPC_Decode, 210, 7, 52, // Opcode: LUi
/* 1336 */    MCD_OPC_FilterValue, 16, 220, 0, // Skip to: 1560
/* 1340 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 1343 */    MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 1361
/* 1347 */    MCD_OPC_CheckPredicate, 10, 87, 48, // Skip to: 13726
/* 1351 */    MCD_OPC_CheckField, 3, 8, 0, 81, 48, // Skip to: 13726
/* 1357 */    MCD_OPC_Decode, 179, 8, 81, // Opcode: MFC0
/* 1361 */    MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 1379
/* 1365 */    MCD_OPC_CheckPredicate, 10, 69, 48, // Skip to: 13726
/* 1369 */    MCD_OPC_CheckField, 3, 8, 0, 63, 48, // Skip to: 13726
/* 1375 */    MCD_OPC_Decode, 169, 9, 81, // Opcode: MTC0
/* 1379 */    MCD_OPC_FilterValue, 8, 51, 0, // Skip to: 1434
/* 1383 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 1386 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1398
/* 1390 */    MCD_OPC_CheckPredicate, 13, 44, 48, // Skip to: 13726
/* 1394 */    MCD_OPC_Decode, 176, 1, 82, // Opcode: BC0F
/* 1398 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1410
/* 1402 */    MCD_OPC_CheckPredicate, 13, 32, 48, // Skip to: 13726
/* 1406 */    MCD_OPC_Decode, 178, 1, 82, // Opcode: BC0T
/* 1410 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1422
/* 1414 */    MCD_OPC_CheckPredicate, 13, 20, 48, // Skip to: 13726
/* 1418 */    MCD_OPC_Decode, 177, 1, 82, // Opcode: BC0FL
/* 1422 */    MCD_OPC_FilterValue, 3, 12, 48, // Skip to: 13726
/* 1426 */    MCD_OPC_CheckPredicate, 13, 8, 48, // Skip to: 13726
/* 1430 */    MCD_OPC_Decode, 179, 1, 82, // Opcode: BC0TL
/* 1434 */    MCD_OPC_FilterValue, 11, 31, 0, // Skip to: 1469
/* 1438 */    MCD_OPC_ExtractField, 0, 16,  // Inst{15-0} ...
/* 1441 */    MCD_OPC_FilterValue, 128, 192, 1, 8, 0, // Skip to: 1455
/* 1447 */    MCD_OPC_CheckPredicate, 6, 243, 47, // Skip to: 13726
/* 1451 */    MCD_OPC_Decode, 179, 4, 42, // Opcode: DI
/* 1455 */    MCD_OPC_FilterValue, 160, 192, 1, 233, 47, // Skip to: 13726
/* 1461 */    MCD_OPC_CheckPredicate, 6, 229, 47, // Skip to: 13726
/* 1465 */    MCD_OPC_Decode, 141, 5, 42, // Opcode: EI
/* 1469 */    MCD_OPC_FilterValue, 16, 221, 47, // Skip to: 13726
/* 1473 */    MCD_OPC_ExtractField, 0, 21,  // Inst{20-0} ...
/* 1476 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1488
/* 1480 */    MCD_OPC_CheckPredicate, 5, 210, 47, // Skip to: 13726
/* 1484 */    MCD_OPC_Decode, 196, 13, 0, // Opcode: TLBR
/* 1488 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1500
/* 1492 */    MCD_OPC_CheckPredicate, 5, 198, 47, // Skip to: 13726
/* 1496 */    MCD_OPC_Decode, 198, 13, 0, // Opcode: TLBWI
/* 1500 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 1512
/* 1504 */    MCD_OPC_CheckPredicate, 5, 186, 47, // Skip to: 13726
/* 1508 */    MCD_OPC_Decode, 200, 13, 0, // Opcode: TLBWR
/* 1512 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1524
/* 1516 */    MCD_OPC_CheckPredicate, 5, 174, 47, // Skip to: 13726
/* 1520 */    MCD_OPC_Decode, 194, 13, 0, // Opcode: TLBP
/* 1524 */    MCD_OPC_FilterValue, 24, 8, 0, // Skip to: 1536
/* 1528 */    MCD_OPC_CheckPredicate, 17, 162, 47, // Skip to: 13726
/* 1532 */    MCD_OPC_Decode, 143, 5, 0, // Opcode: ERET
/* 1536 */    MCD_OPC_FilterValue, 31, 8, 0, // Skip to: 1548
/* 1540 */    MCD_OPC_CheckPredicate, 10, 150, 47, // Skip to: 13726
/* 1544 */    MCD_OPC_Decode, 174, 4, 0, // Opcode: DERET
/* 1548 */    MCD_OPC_FilterValue, 32, 142, 47, // Skip to: 13726
/* 1552 */    MCD_OPC_CheckPredicate, 18, 138, 47, // Skip to: 13726
/* 1556 */    MCD_OPC_Decode, 231, 13, 0, // Opcode: WAIT
/* 1560 */    MCD_OPC_FilterValue, 17, 21, 6, // Skip to: 3121
/* 1564 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 1567 */    MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 1585
/* 1571 */    MCD_OPC_CheckPredicate, 5, 119, 47, // Skip to: 13726
/* 1575 */    MCD_OPC_CheckField, 0, 11, 0, 113, 47, // Skip to: 13726
/* 1581 */    MCD_OPC_Decode, 180, 8, 83, // Opcode: MFC1
/* 1585 */    MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 1603
/* 1589 */    MCD_OPC_CheckPredicate, 19, 101, 47, // Skip to: 13726
/* 1593 */    MCD_OPC_CheckField, 0, 11, 0, 95, 47, // Skip to: 13726
/* 1599 */    MCD_OPC_Decode, 197, 4, 84, // Opcode: DMFC1
/* 1603 */    MCD_OPC_FilterValue, 2, 14, 0, // Skip to: 1621
/* 1607 */    MCD_OPC_CheckPredicate, 5, 83, 47, // Skip to: 13726
/* 1611 */    MCD_OPC_CheckField, 0, 11, 0, 77, 47, // Skip to: 13726
/* 1617 */    MCD_OPC_Decode, 238, 2, 85, // Opcode: CFC1
/* 1621 */    MCD_OPC_FilterValue, 3, 14, 0, // Skip to: 1639
/* 1625 */    MCD_OPC_CheckPredicate, 20, 65, 47, // Skip to: 13726
/* 1629 */    MCD_OPC_CheckField, 0, 11, 0, 59, 47, // Skip to: 13726
/* 1635 */    MCD_OPC_Decode, 183, 8, 86, // Opcode: MFHC1_D32
/* 1639 */    MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 1657
/* 1643 */    MCD_OPC_CheckPredicate, 5, 47, 47, // Skip to: 13726
/* 1647 */    MCD_OPC_CheckField, 0, 11, 0, 41, 47, // Skip to: 13726
/* 1653 */    MCD_OPC_Decode, 170, 9, 87, // Opcode: MTC1
/* 1657 */    MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 1675
/* 1661 */    MCD_OPC_CheckPredicate, 19, 29, 47, // Skip to: 13726
/* 1665 */    MCD_OPC_CheckField, 0, 11, 0, 23, 47, // Skip to: 13726
/* 1671 */    MCD_OPC_Decode, 202, 4, 88, // Opcode: DMTC1
/* 1675 */    MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 1693
/* 1679 */    MCD_OPC_CheckPredicate, 5, 11, 47, // Skip to: 13726
/* 1683 */    MCD_OPC_CheckField, 0, 11, 0, 5, 47, // Skip to: 13726
/* 1689 */    MCD_OPC_Decode, 210, 3, 89, // Opcode: CTC1
/* 1693 */    MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 1711
/* 1697 */    MCD_OPC_CheckPredicate, 20, 249, 46, // Skip to: 13726
/* 1701 */    MCD_OPC_CheckField, 0, 11, 0, 243, 46, // Skip to: 13726
/* 1707 */    MCD_OPC_Decode, 173, 9, 90, // Opcode: MTHC1_D32
/* 1711 */    MCD_OPC_FilterValue, 8, 51, 0, // Skip to: 1766
/* 1715 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 1718 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1730
/* 1722 */    MCD_OPC_CheckPredicate, 13, 224, 46, // Skip to: 13726
/* 1726 */    MCD_OPC_Decode, 181, 1, 91, // Opcode: BC1F
/* 1730 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1742
/* 1734 */    MCD_OPC_CheckPredicate, 13, 212, 46, // Skip to: 13726
/* 1738 */    MCD_OPC_Decode, 185, 1, 91, // Opcode: BC1T
/* 1742 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1754
/* 1746 */    MCD_OPC_CheckPredicate, 16, 200, 46, // Skip to: 13726
/* 1750 */    MCD_OPC_Decode, 182, 1, 91, // Opcode: BC1FL
/* 1754 */    MCD_OPC_FilterValue, 3, 192, 46, // Skip to: 13726
/* 1758 */    MCD_OPC_CheckPredicate, 16, 188, 46, // Skip to: 13726
/* 1762 */    MCD_OPC_Decode, 186, 1, 91, // Opcode: BC1TL
/* 1766 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1778
/* 1770 */    MCD_OPC_CheckPredicate, 8, 176, 46, // Skip to: 13726
/* 1774 */    MCD_OPC_Decode, 192, 2, 92, // Opcode: BZ_V
/* 1778 */    MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 1790
/* 1782 */    MCD_OPC_CheckPredicate, 8, 164, 46, // Skip to: 13726
/* 1786 */    MCD_OPC_Decode, 166, 2, 92, // Opcode: BNZ_V
/* 1790 */    MCD_OPC_FilterValue, 16, 80, 2, // Skip to: 2386
/* 1794 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 1797 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1809
/* 1801 */    MCD_OPC_CheckPredicate, 5, 145, 46, // Skip to: 13726
/* 1805 */    MCD_OPC_Decode, 174, 5, 93, // Opcode: FADD_S
/* 1809 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1821
/* 1813 */    MCD_OPC_CheckPredicate, 5, 133, 46, // Skip to: 13726
/* 1817 */    MCD_OPC_Decode, 176, 6, 93, // Opcode: FSUB_S
/* 1821 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1833
/* 1825 */    MCD_OPC_CheckPredicate, 5, 121, 46, // Skip to: 13726
/* 1829 */    MCD_OPC_Decode, 139, 6, 93, // Opcode: FMUL_S
/* 1833 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1845
/* 1837 */    MCD_OPC_CheckPredicate, 5, 109, 46, // Skip to: 13726
/* 1841 */    MCD_OPC_Decode, 210, 5, 93, // Opcode: FDIV_S
/* 1845 */    MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 1863
/* 1849 */    MCD_OPC_CheckPredicate, 15, 97, 46, // Skip to: 13726
/* 1853 */    MCD_OPC_CheckField, 16, 5, 0, 91, 46, // Skip to: 13726
/* 1859 */    MCD_OPC_Decode, 169, 6, 94, // Opcode: FSQRT_S
/* 1863 */    MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 1881
/* 1867 */    MCD_OPC_CheckPredicate, 5, 79, 46, // Skip to: 13726
/* 1871 */    MCD_OPC_CheckField, 16, 5, 0, 73, 46, // Skip to: 13726
/* 1877 */    MCD_OPC_Decode, 167, 5, 94, // Opcode: FABS_S
/* 1881 */    MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 1899
/* 1885 */    MCD_OPC_CheckPredicate, 5, 61, 46, // Skip to: 13726
/* 1889 */    MCD_OPC_CheckField, 16, 5, 0, 55, 46, // Skip to: 13726
/* 1895 */    MCD_OPC_Decode, 131, 6, 94, // Opcode: FMOV_S
/* 1899 */    MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 1917
/* 1903 */    MCD_OPC_CheckPredicate, 5, 43, 46, // Skip to: 13726
/* 1907 */    MCD_OPC_CheckField, 16, 5, 0, 37, 46, // Skip to: 13726
/* 1913 */    MCD_OPC_Decode, 145, 6, 94, // Opcode: FNEG_S
/* 1917 */    MCD_OPC_FilterValue, 12, 14, 0, // Skip to: 1935
/* 1921 */    MCD_OPC_CheckPredicate, 15, 25, 46, // Skip to: 13726
/* 1925 */    MCD_OPC_CheckField, 16, 5, 0, 19, 46, // Skip to: 13726
/* 1931 */    MCD_OPC_Decode, 128, 11, 94, // Opcode: ROUND_W_S
/* 1935 */    MCD_OPC_FilterValue, 13, 14, 0, // Skip to: 1953
/* 1939 */    MCD_OPC_CheckPredicate, 15, 7, 46, // Skip to: 13726
/* 1943 */    MCD_OPC_CheckField, 16, 5, 0, 1, 46, // Skip to: 13726
/* 1949 */    MCD_OPC_Decode, 219, 13, 94, // Opcode: TRUNC_W_S
/* 1953 */    MCD_OPC_FilterValue, 14, 14, 0, // Skip to: 1971
/* 1957 */    MCD_OPC_CheckPredicate, 15, 245, 45, // Skip to: 13726
/* 1961 */    MCD_OPC_CheckField, 16, 5, 0, 239, 45, // Skip to: 13726
/* 1967 */    MCD_OPC_Decode, 228, 2, 94, // Opcode: CEIL_W_S
/* 1971 */    MCD_OPC_FilterValue, 15, 14, 0, // Skip to: 1989
/* 1975 */    MCD_OPC_CheckPredicate, 15, 227, 45, // Skip to: 13726
/* 1979 */    MCD_OPC_CheckField, 16, 5, 0, 221, 45, // Skip to: 13726
/* 1985 */    MCD_OPC_Decode, 244, 5, 94, // Opcode: FLOOR_W_S
/* 1989 */    MCD_OPC_FilterValue, 17, 27, 0, // Skip to: 2020
/* 1993 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 1996 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 2008
/* 2000 */    MCD_OPC_CheckPredicate, 7, 202, 45, // Skip to: 13726
/* 2004 */    MCD_OPC_Decode, 242, 8, 95, // Opcode: MOVF_S
/* 2008 */    MCD_OPC_FilterValue, 1, 194, 45, // Skip to: 13726
/* 2012 */    MCD_OPC_CheckPredicate, 7, 190, 45, // Skip to: 13726
/* 2016 */    MCD_OPC_Decode, 134, 9, 95, // Opcode: MOVT_S
/* 2020 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 2032
/* 2024 */    MCD_OPC_CheckPredicate, 7, 178, 45, // Skip to: 13726
/* 2028 */    MCD_OPC_Decode, 146, 9, 96, // Opcode: MOVZ_I_S
/* 2032 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 2044
/* 2036 */    MCD_OPC_CheckPredicate, 7, 166, 45, // Skip to: 13726
/* 2040 */    MCD_OPC_Decode, 254, 8, 96, // Opcode: MOVN_I_S
/* 2044 */    MCD_OPC_FilterValue, 33, 14, 0, // Skip to: 2062
/* 2048 */    MCD_OPC_CheckPredicate, 21, 154, 45, // Skip to: 13726
/* 2052 */    MCD_OPC_CheckField, 16, 5, 0, 148, 45, // Skip to: 13726
/* 2058 */    MCD_OPC_Decode, 213, 3, 97, // Opcode: CVT_D32_S
/* 2062 */    MCD_OPC_FilterValue, 36, 14, 0, // Skip to: 2080
/* 2066 */    MCD_OPC_CheckPredicate, 5, 136, 45, // Skip to: 13726
/* 2070 */    MCD_OPC_CheckField, 16, 5, 0, 130, 45, // Skip to: 13726
/* 2076 */    MCD_OPC_Decode, 233, 3, 94, // Opcode: CVT_W_S
/* 2080 */    MCD_OPC_FilterValue, 37, 14, 0, // Skip to: 2098
/* 2084 */    MCD_OPC_CheckPredicate, 22, 118, 45, // Skip to: 13726
/* 2088 */    MCD_OPC_CheckField, 16, 5, 0, 112, 45, // Skip to: 13726
/* 2094 */    MCD_OPC_Decode, 222, 3, 98, // Opcode: CVT_L_S
/* 2098 */    MCD_OPC_FilterValue, 48, 14, 0, // Skip to: 2116
/* 2102 */    MCD_OPC_CheckPredicate, 13, 100, 45, // Skip to: 13726
/* 2106 */    MCD_OPC_CheckField, 6, 5, 0, 94, 45, // Skip to: 13726
/* 2112 */    MCD_OPC_Decode, 240, 3, 99, // Opcode: C_F_S
/* 2116 */    MCD_OPC_FilterValue, 49, 14, 0, // Skip to: 2134
/* 2120 */    MCD_OPC_CheckPredicate, 13, 82, 45, // Skip to: 13726
/* 2124 */    MCD_OPC_CheckField, 6, 5, 0, 76, 45, // Skip to: 13726
/* 2130 */    MCD_OPC_Decode, 154, 4, 99, // Opcode: C_UN_S
/* 2134 */    MCD_OPC_FilterValue, 50, 14, 0, // Skip to: 2152
/* 2138 */    MCD_OPC_CheckPredicate, 13, 64, 45, // Skip to: 13726
/* 2142 */    MCD_OPC_CheckField, 6, 5, 0, 58, 45, // Skip to: 13726
/* 2148 */    MCD_OPC_Decode, 237, 3, 99, // Opcode: C_EQ_S
/* 2152 */    MCD_OPC_FilterValue, 51, 14, 0, // Skip to: 2170
/* 2156 */    MCD_OPC_CheckPredicate, 13, 46, 45, // Skip to: 13726
/* 2160 */    MCD_OPC_CheckField, 6, 5, 0, 40, 45, // Skip to: 13726
/* 2166 */    MCD_OPC_Decode, 145, 4, 99, // Opcode: C_UEQ_S
/* 2170 */    MCD_OPC_FilterValue, 52, 14, 0, // Skip to: 2188
/* 2174 */    MCD_OPC_CheckPredicate, 13, 28, 45, // Skip to: 13726
/* 2178 */    MCD_OPC_CheckField, 6, 5, 0, 22, 45, // Skip to: 13726
/* 2184 */    MCD_OPC_Decode, 136, 4, 99, // Opcode: C_OLT_S
/* 2188 */    MCD_OPC_FilterValue, 53, 14, 0, // Skip to: 2206
/* 2192 */    MCD_OPC_CheckPredicate, 13, 10, 45, // Skip to: 13726
/* 2196 */    MCD_OPC_CheckField, 6, 5, 0, 4, 45, // Skip to: 13726
/* 2202 */    MCD_OPC_Decode, 151, 4, 99, // Opcode: C_ULT_S
/* 2206 */    MCD_OPC_FilterValue, 54, 14, 0, // Skip to: 2224
/* 2210 */    MCD_OPC_CheckPredicate, 13, 248, 44, // Skip to: 13726
/* 2214 */    MCD_OPC_CheckField, 6, 5, 0, 242, 44, // Skip to: 13726
/* 2220 */    MCD_OPC_Decode, 133, 4, 99, // Opcode: C_OLE_S
/* 2224 */    MCD_OPC_FilterValue, 55, 14, 0, // Skip to: 2242
/* 2228 */    MCD_OPC_CheckPredicate, 13, 230, 44, // Skip to: 13726
/* 2232 */    MCD_OPC_CheckField, 6, 5, 0, 224, 44, // Skip to: 13726
/* 2238 */    MCD_OPC_Decode, 148, 4, 99, // Opcode: C_ULE_S
/* 2242 */    MCD_OPC_FilterValue, 56, 14, 0, // Skip to: 2260
/* 2246 */    MCD_OPC_CheckPredicate, 13, 212, 44, // Skip to: 13726
/* 2250 */    MCD_OPC_CheckField, 6, 5, 0, 206, 44, // Skip to: 13726
/* 2256 */    MCD_OPC_Decode, 142, 4, 99, // Opcode: C_SF_S
/* 2260 */    MCD_OPC_FilterValue, 57, 14, 0, // Skip to: 2278
/* 2264 */    MCD_OPC_CheckPredicate, 13, 194, 44, // Skip to: 13726
/* 2268 */    MCD_OPC_CheckField, 6, 5, 0, 188, 44, // Skip to: 13726
/* 2274 */    MCD_OPC_Decode, 252, 3, 99, // Opcode: C_NGLE_S
/* 2278 */    MCD_OPC_FilterValue, 58, 14, 0, // Skip to: 2296
/* 2282 */    MCD_OPC_CheckPredicate, 13, 176, 44, // Skip to: 13726
/* 2286 */    MCD_OPC_CheckField, 6, 5, 0, 170, 44, // Skip to: 13726
/* 2292 */    MCD_OPC_Decode, 139, 4, 99, // Opcode: C_SEQ_S
/* 2296 */    MCD_OPC_FilterValue, 59, 14, 0, // Skip to: 2314
/* 2300 */    MCD_OPC_CheckPredicate, 13, 158, 44, // Skip to: 13726
/* 2304 */    MCD_OPC_CheckField, 6, 5, 0, 152, 44, // Skip to: 13726
/* 2310 */    MCD_OPC_Decode, 255, 3, 99, // Opcode: C_NGL_S
/* 2314 */    MCD_OPC_FilterValue, 60, 14, 0, // Skip to: 2332
/* 2318 */    MCD_OPC_CheckPredicate, 13, 140, 44, // Skip to: 13726
/* 2322 */    MCD_OPC_CheckField, 6, 5, 0, 134, 44, // Skip to: 13726
/* 2328 */    MCD_OPC_Decode, 246, 3, 99, // Opcode: C_LT_S
/* 2332 */    MCD_OPC_FilterValue, 61, 14, 0, // Skip to: 2350
/* 2336 */    MCD_OPC_CheckPredicate, 13, 122, 44, // Skip to: 13726
/* 2340 */    MCD_OPC_CheckField, 6, 5, 0, 116, 44, // Skip to: 13726
/* 2346 */    MCD_OPC_Decode, 249, 3, 99, // Opcode: C_NGE_S
/* 2350 */    MCD_OPC_FilterValue, 62, 14, 0, // Skip to: 2368
/* 2354 */    MCD_OPC_CheckPredicate, 13, 104, 44, // Skip to: 13726
/* 2358 */    MCD_OPC_CheckField, 6, 5, 0, 98, 44, // Skip to: 13726
/* 2364 */    MCD_OPC_Decode, 243, 3, 99, // Opcode: C_LE_S
/* 2368 */    MCD_OPC_FilterValue, 63, 90, 44, // Skip to: 13726
/* 2372 */    MCD_OPC_CheckPredicate, 13, 86, 44, // Skip to: 13726
/* 2376 */    MCD_OPC_CheckField, 6, 5, 0, 80, 44, // Skip to: 13726
/* 2382 */    MCD_OPC_Decode, 130, 4, 99, // Opcode: C_NGT_S
/* 2386 */    MCD_OPC_FilterValue, 17, 80, 2, // Skip to: 2982
/* 2390 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 2393 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 2405
/* 2397 */    MCD_OPC_CheckPredicate, 21, 61, 44, // Skip to: 13726
/* 2401 */    MCD_OPC_Decode, 171, 5, 100, // Opcode: FADD_D32
/* 2405 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 2417
/* 2409 */    MCD_OPC_CheckPredicate, 21, 49, 44, // Skip to: 13726
/* 2413 */    MCD_OPC_Decode, 173, 6, 100, // Opcode: FSUB_D32
/* 2417 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 2429
/* 2421 */    MCD_OPC_CheckPredicate, 21, 37, 44, // Skip to: 13726
/* 2425 */    MCD_OPC_Decode, 136, 6, 100, // Opcode: FMUL_D32
/* 2429 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 2441
/* 2433 */    MCD_OPC_CheckPredicate, 21, 25, 44, // Skip to: 13726
/* 2437 */    MCD_OPC_Decode, 207, 5, 100, // Opcode: FDIV_D32
/* 2441 */    MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 2459
/* 2445 */    MCD_OPC_CheckPredicate, 23, 13, 44, // Skip to: 13726
/* 2449 */    MCD_OPC_CheckField, 16, 5, 0, 7, 44, // Skip to: 13726
/* 2455 */    MCD_OPC_Decode, 166, 6, 101, // Opcode: FSQRT_D32
/* 2459 */    MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 2477
/* 2463 */    MCD_OPC_CheckPredicate, 21, 251, 43, // Skip to: 13726
/* 2467 */    MCD_OPC_CheckField, 16, 5, 0, 245, 43, // Skip to: 13726
/* 2473 */    MCD_OPC_Decode, 164, 5, 101, // Opcode: FABS_D32
/* 2477 */    MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 2495
/* 2481 */    MCD_OPC_CheckPredicate, 21, 233, 43, // Skip to: 13726
/* 2485 */    MCD_OPC_CheckField, 16, 5, 0, 227, 43, // Skip to: 13726
/* 2491 */    MCD_OPC_Decode, 128, 6, 101, // Opcode: FMOV_D32
/* 2495 */    MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 2513
/* 2499 */    MCD_OPC_CheckPredicate, 21, 215, 43, // Skip to: 13726
/* 2503 */    MCD_OPC_CheckField, 16, 5, 0, 209, 43, // Skip to: 13726
/* 2509 */    MCD_OPC_Decode, 142, 6, 101, // Opcode: FNEG_D32
/* 2513 */    MCD_OPC_FilterValue, 12, 14, 0, // Skip to: 2531
/* 2517 */    MCD_OPC_CheckPredicate, 23, 197, 43, // Skip to: 13726
/* 2521 */    MCD_OPC_CheckField, 16, 5, 0, 191, 43, // Skip to: 13726
/* 2527 */    MCD_OPC_Decode, 253, 10, 102, // Opcode: ROUND_W_D32
/* 2531 */    MCD_OPC_FilterValue, 13, 14, 0, // Skip to: 2549
/* 2535 */    MCD_OPC_CheckPredicate, 23, 179, 43, // Skip to: 13726
/* 2539 */    MCD_OPC_CheckField, 16, 5, 0, 173, 43, // Skip to: 13726
/* 2545 */    MCD_OPC_Decode, 216, 13, 102, // Opcode: TRUNC_W_D32
/* 2549 */    MCD_OPC_FilterValue, 14, 14, 0, // Skip to: 2567
/* 2553 */    MCD_OPC_CheckPredicate, 23, 161, 43, // Skip to: 13726
/* 2557 */    MCD_OPC_CheckField, 16, 5, 0, 155, 43, // Skip to: 13726
/* 2563 */    MCD_OPC_Decode, 225, 2, 102, // Opcode: CEIL_W_D32
/* 2567 */    MCD_OPC_FilterValue, 15, 14, 0, // Skip to: 2585
/* 2571 */    MCD_OPC_CheckPredicate, 23, 143, 43, // Skip to: 13726
/* 2575 */    MCD_OPC_CheckField, 16, 5, 0, 137, 43, // Skip to: 13726
/* 2581 */    MCD_OPC_Decode, 241, 5, 102, // Opcode: FLOOR_W_D32
/* 2585 */    MCD_OPC_FilterValue, 17, 27, 0, // Skip to: 2616
/* 2589 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 2592 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 2604
/* 2596 */    MCD_OPC_CheckPredicate, 24, 118, 43, // Skip to: 13726
/* 2600 */    MCD_OPC_Decode, 236, 8, 103, // Opcode: MOVF_D32
/* 2604 */    MCD_OPC_FilterValue, 1, 110, 43, // Skip to: 13726
/* 2608 */    MCD_OPC_CheckPredicate, 24, 106, 43, // Skip to: 13726
/* 2612 */    MCD_OPC_Decode, 128, 9, 103, // Opcode: MOVT_D32
/* 2616 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 2628
/* 2620 */    MCD_OPC_CheckPredicate, 24, 94, 43, // Skip to: 13726
/* 2624 */    MCD_OPC_Decode, 140, 9, 104, // Opcode: MOVZ_I_D32
/* 2628 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 2640
/* 2632 */    MCD_OPC_CheckPredicate, 24, 82, 43, // Skip to: 13726
/* 2636 */    MCD_OPC_Decode, 248, 8, 104, // Opcode: MOVN_I_D32
/* 2640 */    MCD_OPC_FilterValue, 32, 14, 0, // Skip to: 2658
/* 2644 */    MCD_OPC_CheckPredicate, 21, 70, 43, // Skip to: 13726
/* 2648 */    MCD_OPC_CheckField, 16, 5, 0, 64, 43, // Skip to: 13726
/* 2654 */    MCD_OPC_Decode, 224, 3, 102, // Opcode: CVT_S_D32
/* 2658 */    MCD_OPC_FilterValue, 36, 14, 0, // Skip to: 2676
/* 2662 */    MCD_OPC_CheckPredicate, 21, 52, 43, // Skip to: 13726
/* 2666 */    MCD_OPC_CheckField, 16, 5, 0, 46, 43, // Skip to: 13726
/* 2672 */    MCD_OPC_Decode, 230, 3, 102, // Opcode: CVT_W_D32
/* 2676 */    MCD_OPC_FilterValue, 37, 14, 0, // Skip to: 2694
/* 2680 */    MCD_OPC_CheckPredicate, 22, 34, 43, // Skip to: 13726
/* 2684 */    MCD_OPC_CheckField, 16, 5, 0, 28, 43, // Skip to: 13726
/* 2690 */    MCD_OPC_Decode, 220, 3, 105, // Opcode: CVT_L_D64
/* 2694 */    MCD_OPC_FilterValue, 48, 14, 0, // Skip to: 2712
/* 2698 */    MCD_OPC_CheckPredicate, 25, 16, 43, // Skip to: 13726
/* 2702 */    MCD_OPC_CheckField, 6, 5, 0, 10, 43, // Skip to: 13726
/* 2708 */    MCD_OPC_Decode, 238, 3, 106, // Opcode: C_F_D32
/* 2712 */    MCD_OPC_FilterValue, 49, 14, 0, // Skip to: 2730
/* 2716 */    MCD_OPC_CheckPredicate, 25, 254, 42, // Skip to: 13726
/* 2720 */    MCD_OPC_CheckField, 6, 5, 0, 248, 42, // Skip to: 13726
/* 2726 */    MCD_OPC_Decode, 152, 4, 106, // Opcode: C_UN_D32
/* 2730 */    MCD_OPC_FilterValue, 50, 14, 0, // Skip to: 2748
/* 2734 */    MCD_OPC_CheckPredicate, 25, 236, 42, // Skip to: 13726
/* 2738 */    MCD_OPC_CheckField, 6, 5, 0, 230, 42, // Skip to: 13726
/* 2744 */    MCD_OPC_Decode, 235, 3, 106, // Opcode: C_EQ_D32
/* 2748 */    MCD_OPC_FilterValue, 51, 14, 0, // Skip to: 2766
/* 2752 */    MCD_OPC_CheckPredicate, 25, 218, 42, // Skip to: 13726
/* 2756 */    MCD_OPC_CheckField, 6, 5, 0, 212, 42, // Skip to: 13726
/* 2762 */    MCD_OPC_Decode, 143, 4, 106, // Opcode: C_UEQ_D32
/* 2766 */    MCD_OPC_FilterValue, 52, 14, 0, // Skip to: 2784
/* 2770 */    MCD_OPC_CheckPredicate, 25, 200, 42, // Skip to: 13726
/* 2774 */    MCD_OPC_CheckField, 6, 5, 0, 194, 42, // Skip to: 13726
/* 2780 */    MCD_OPC_Decode, 134, 4, 106, // Opcode: C_OLT_D32
/* 2784 */    MCD_OPC_FilterValue, 53, 14, 0, // Skip to: 2802
/* 2788 */    MCD_OPC_CheckPredicate, 25, 182, 42, // Skip to: 13726
/* 2792 */    MCD_OPC_CheckField, 6, 5, 0, 176, 42, // Skip to: 13726
/* 2798 */    MCD_OPC_Decode, 149, 4, 106, // Opcode: C_ULT_D32
/* 2802 */    MCD_OPC_FilterValue, 54, 14, 0, // Skip to: 2820
/* 2806 */    MCD_OPC_CheckPredicate, 25, 164, 42, // Skip to: 13726
/* 2810 */    MCD_OPC_CheckField, 6, 5, 0, 158, 42, // Skip to: 13726
/* 2816 */    MCD_OPC_Decode, 131, 4, 106, // Opcode: C_OLE_D32
/* 2820 */    MCD_OPC_FilterValue, 55, 14, 0, // Skip to: 2838
/* 2824 */    MCD_OPC_CheckPredicate, 25, 146, 42, // Skip to: 13726
/* 2828 */    MCD_OPC_CheckField, 6, 5, 0, 140, 42, // Skip to: 13726
/* 2834 */    MCD_OPC_Decode, 146, 4, 106, // Opcode: C_ULE_D32
/* 2838 */    MCD_OPC_FilterValue, 56, 14, 0, // Skip to: 2856
/* 2842 */    MCD_OPC_CheckPredicate, 25, 128, 42, // Skip to: 13726
/* 2846 */    MCD_OPC_CheckField, 6, 5, 0, 122, 42, // Skip to: 13726
/* 2852 */    MCD_OPC_Decode, 140, 4, 106, // Opcode: C_SF_D32
/* 2856 */    MCD_OPC_FilterValue, 57, 14, 0, // Skip to: 2874
/* 2860 */    MCD_OPC_CheckPredicate, 25, 110, 42, // Skip to: 13726
/* 2864 */    MCD_OPC_CheckField, 6, 5, 0, 104, 42, // Skip to: 13726
/* 2870 */    MCD_OPC_Decode, 250, 3, 106, // Opcode: C_NGLE_D32
/* 2874 */    MCD_OPC_FilterValue, 58, 14, 0, // Skip to: 2892
/* 2878 */    MCD_OPC_CheckPredicate, 25, 92, 42, // Skip to: 13726
/* 2882 */    MCD_OPC_CheckField, 6, 5, 0, 86, 42, // Skip to: 13726
/* 2888 */    MCD_OPC_Decode, 137, 4, 106, // Opcode: C_SEQ_D32
/* 2892 */    MCD_OPC_FilterValue, 59, 14, 0, // Skip to: 2910
/* 2896 */    MCD_OPC_CheckPredicate, 25, 74, 42, // Skip to: 13726
/* 2900 */    MCD_OPC_CheckField, 6, 5, 0, 68, 42, // Skip to: 13726
/* 2906 */    MCD_OPC_Decode, 253, 3, 106, // Opcode: C_NGL_D32
/* 2910 */    MCD_OPC_FilterValue, 60, 14, 0, // Skip to: 2928
/* 2914 */    MCD_OPC_CheckPredicate, 25, 56, 42, // Skip to: 13726
/* 2918 */    MCD_OPC_CheckField, 6, 5, 0, 50, 42, // Skip to: 13726
/* 2924 */    MCD_OPC_Decode, 244, 3, 106, // Opcode: C_LT_D32
/* 2928 */    MCD_OPC_FilterValue, 61, 14, 0, // Skip to: 2946
/* 2932 */    MCD_OPC_CheckPredicate, 25, 38, 42, // Skip to: 13726
/* 2936 */    MCD_OPC_CheckField, 6, 5, 0, 32, 42, // Skip to: 13726
/* 2942 */    MCD_OPC_Decode, 247, 3, 106, // Opcode: C_NGE_D32
/* 2946 */    MCD_OPC_FilterValue, 62, 14, 0, // Skip to: 2964
/* 2950 */    MCD_OPC_CheckPredicate, 25, 20, 42, // Skip to: 13726
/* 2954 */    MCD_OPC_CheckField, 6, 5, 0, 14, 42, // Skip to: 13726
/* 2960 */    MCD_OPC_Decode, 241, 3, 106, // Opcode: C_LE_D32
/* 2964 */    MCD_OPC_FilterValue, 63, 6, 42, // Skip to: 13726
/* 2968 */    MCD_OPC_CheckPredicate, 25, 2, 42, // Skip to: 13726
/* 2972 */    MCD_OPC_CheckField, 6, 5, 0, 252, 41, // Skip to: 13726
/* 2978 */    MCD_OPC_Decode, 128, 4, 106, // Opcode: C_NGT_D32
/* 2982 */    MCD_OPC_FilterValue, 20, 39, 0, // Skip to: 3025
/* 2986 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 2989 */    MCD_OPC_FilterValue, 32, 14, 0, // Skip to: 3007
/* 2993 */    MCD_OPC_CheckPredicate, 5, 233, 41, // Skip to: 13726
/* 2997 */    MCD_OPC_CheckField, 16, 5, 0, 227, 41, // Skip to: 13726
/* 3003 */    MCD_OPC_Decode, 228, 3, 94, // Opcode: CVT_S_W
/* 3007 */    MCD_OPC_FilterValue, 33, 219, 41, // Skip to: 13726
/* 3011 */    MCD_OPC_CheckPredicate, 21, 215, 41, // Skip to: 13726
/* 3015 */    MCD_OPC_CheckField, 16, 5, 0, 209, 41, // Skip to: 13726
/* 3021 */    MCD_OPC_Decode, 214, 3, 97, // Opcode: CVT_D32_W
/* 3025 */    MCD_OPC_FilterValue, 24, 8, 0, // Skip to: 3037
/* 3029 */    MCD_OPC_CheckPredicate, 8, 197, 41, // Skip to: 13726
/* 3033 */    MCD_OPC_Decode, 189, 2, 92, // Opcode: BZ_B
/* 3037 */    MCD_OPC_FilterValue, 25, 8, 0, // Skip to: 3049
/* 3041 */    MCD_OPC_CheckPredicate, 8, 185, 41, // Skip to: 13726
/* 3045 */    MCD_OPC_Decode, 191, 2, 107, // Opcode: BZ_H
/* 3049 */    MCD_OPC_FilterValue, 26, 8, 0, // Skip to: 3061
/* 3053 */    MCD_OPC_CheckPredicate, 8, 173, 41, // Skip to: 13726
/* 3057 */    MCD_OPC_Decode, 193, 2, 108, // Opcode: BZ_W
/* 3061 */    MCD_OPC_FilterValue, 27, 8, 0, // Skip to: 3073
/* 3065 */    MCD_OPC_CheckPredicate, 8, 161, 41, // Skip to: 13726
/* 3069 */    MCD_OPC_Decode, 190, 2, 109, // Opcode: BZ_D
/* 3073 */    MCD_OPC_FilterValue, 28, 8, 0, // Skip to: 3085
/* 3077 */    MCD_OPC_CheckPredicate, 8, 149, 41, // Skip to: 13726
/* 3081 */    MCD_OPC_Decode, 163, 2, 92, // Opcode: BNZ_B
/* 3085 */    MCD_OPC_FilterValue, 29, 8, 0, // Skip to: 3097
/* 3089 */    MCD_OPC_CheckPredicate, 8, 137, 41, // Skip to: 13726
/* 3093 */    MCD_OPC_Decode, 165, 2, 107, // Opcode: BNZ_H
/* 3097 */    MCD_OPC_FilterValue, 30, 8, 0, // Skip to: 3109
/* 3101 */    MCD_OPC_CheckPredicate, 8, 125, 41, // Skip to: 13726
/* 3105 */    MCD_OPC_Decode, 167, 2, 108, // Opcode: BNZ_W
/* 3109 */    MCD_OPC_FilterValue, 31, 117, 41, // Skip to: 13726
/* 3113 */    MCD_OPC_CheckPredicate, 8, 113, 41, // Skip to: 13726
/* 3117 */    MCD_OPC_Decode, 164, 2, 109, // Opcode: BNZ_D
/* 3121 */    MCD_OPC_FilterValue, 18, 94, 0, // Skip to: 3219
/* 3125 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 3128 */    MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 3146
/* 3132 */    MCD_OPC_CheckPredicate, 5, 94, 41, // Skip to: 13726
/* 3136 */    MCD_OPC_CheckField, 3, 8, 0, 88, 41, // Skip to: 13726
/* 3142 */    MCD_OPC_Decode, 182, 8, 81, // Opcode: MFC2
/* 3146 */    MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 3164
/* 3150 */    MCD_OPC_CheckPredicate, 5, 76, 41, // Skip to: 13726
/* 3154 */    MCD_OPC_CheckField, 3, 8, 0, 70, 41, // Skip to: 13726
/* 3160 */    MCD_OPC_Decode, 172, 9, 81, // Opcode: MTC2
/* 3164 */    MCD_OPC_FilterValue, 8, 62, 41, // Skip to: 13726
/* 3168 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 3171 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 3183
/* 3175 */    MCD_OPC_CheckPredicate, 13, 51, 41, // Skip to: 13726
/* 3179 */    MCD_OPC_Decode, 189, 1, 82, // Opcode: BC2F
/* 3183 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 3195
/* 3187 */    MCD_OPC_CheckPredicate, 13, 39, 41, // Skip to: 13726
/* 3191 */    MCD_OPC_Decode, 192, 1, 82, // Opcode: BC2T
/* 3195 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 3207
/* 3199 */    MCD_OPC_CheckPredicate, 13, 27, 41, // Skip to: 13726
/* 3203 */    MCD_OPC_Decode, 190, 1, 82, // Opcode: BC2FL
/* 3207 */    MCD_OPC_FilterValue, 3, 19, 41, // Skip to: 13726
/* 3211 */    MCD_OPC_CheckPredicate, 13, 15, 41, // Skip to: 13726
/* 3215 */    MCD_OPC_Decode, 193, 1, 82, // Opcode: BC2TL
/* 3219 */    MCD_OPC_FilterValue, 19, 9, 1, // Skip to: 3488
/* 3223 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 3226 */    MCD_OPC_FilterValue, 8, 51, 0, // Skip to: 3281
/* 3230 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 3233 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 3245
/* 3237 */    MCD_OPC_CheckPredicate, 13, 40, 0, // Skip to: 3281
/* 3241 */    MCD_OPC_Decode, 194, 1, 82, // Opcode: BC3F
/* 3245 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 3257
/* 3249 */    MCD_OPC_CheckPredicate, 13, 28, 0, // Skip to: 3281
/* 3253 */    MCD_OPC_Decode, 196, 1, 82, // Opcode: BC3T
/* 3257 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 3269
/* 3261 */    MCD_OPC_CheckPredicate, 13, 16, 0, // Skip to: 3281
/* 3265 */    MCD_OPC_Decode, 195, 1, 82, // Opcode: BC3FL
/* 3269 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 3281
/* 3273 */    MCD_OPC_CheckPredicate, 13, 4, 0, // Skip to: 3281
/* 3277 */    MCD_OPC_Decode, 197, 1, 82, // Opcode: BC3TL
/* 3281 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 3284 */    MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 3302
/* 3288 */    MCD_OPC_CheckPredicate, 26, 194, 40, // Skip to: 13726
/* 3292 */    MCD_OPC_CheckField, 11, 5, 0, 188, 40, // Skip to: 13726
/* 3298 */    MCD_OPC_Decode, 237, 7, 110, // Opcode: LWXC1
/* 3302 */    MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 3320
/* 3306 */    MCD_OPC_CheckPredicate, 27, 176, 40, // Skip to: 13726
/* 3310 */    MCD_OPC_CheckField, 11, 5, 0, 170, 40, // Skip to: 13726
/* 3316 */    MCD_OPC_Decode, 175, 7, 111, // Opcode: LDXC1
/* 3320 */    MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 3338
/* 3324 */    MCD_OPC_CheckPredicate, 28, 158, 40, // Skip to: 13726
/* 3328 */    MCD_OPC_CheckField, 11, 5, 0, 152, 40, // Skip to: 13726
/* 3334 */    MCD_OPC_Decode, 207, 7, 111, // Opcode: LUXC1
/* 3338 */    MCD_OPC_FilterValue, 8, 14, 0, // Skip to: 3356
/* 3342 */    MCD_OPC_CheckPredicate, 26, 140, 40, // Skip to: 13726
/* 3346 */    MCD_OPC_CheckField, 6, 5, 0, 134, 40, // Skip to: 13726
/* 3352 */    MCD_OPC_Decode, 254, 12, 112, // Opcode: SWXC1
/* 3356 */    MCD_OPC_FilterValue, 9, 14, 0, // Skip to: 3374
/* 3360 */    MCD_OPC_CheckPredicate, 27, 122, 40, // Skip to: 13726
/* 3364 */    MCD_OPC_CheckField, 6, 5, 0, 116, 40, // Skip to: 13726
/* 3370 */    MCD_OPC_Decode, 166, 11, 113, // Opcode: SDXC1
/* 3374 */    MCD_OPC_FilterValue, 13, 14, 0, // Skip to: 3392
/* 3378 */    MCD_OPC_CheckPredicate, 28, 104, 40, // Skip to: 13726
/* 3382 */    MCD_OPC_CheckField, 6, 5, 0, 98, 40, // Skip to: 13726
/* 3388 */    MCD_OPC_Decode, 232, 12, 113, // Opcode: SUXC1
/* 3392 */    MCD_OPC_FilterValue, 32, 8, 0, // Skip to: 3404
/* 3396 */    MCD_OPC_CheckPredicate, 26, 86, 40, // Skip to: 13726
/* 3400 */    MCD_OPC_Decode, 149, 8, 114, // Opcode: MADD_S
/* 3404 */    MCD_OPC_FilterValue, 33, 8, 0, // Skip to: 3416
/* 3408 */    MCD_OPC_CheckPredicate, 29, 74, 40, // Skip to: 13726
/* 3412 */    MCD_OPC_Decode, 142, 8, 115, // Opcode: MADD_D32
/* 3416 */    MCD_OPC_FilterValue, 40, 8, 0, // Skip to: 3428
/* 3420 */    MCD_OPC_CheckPredicate, 26, 62, 40, // Skip to: 13726
/* 3424 */    MCD_OPC_Decode, 167, 9, 114, // Opcode: MSUB_S
/* 3428 */    MCD_OPC_FilterValue, 41, 8, 0, // Skip to: 3440
/* 3432 */    MCD_OPC_CheckPredicate, 29, 50, 40, // Skip to: 13726
/* 3436 */    MCD_OPC_Decode, 160, 9, 115, // Opcode: MSUB_D32
/* 3440 */    MCD_OPC_FilterValue, 48, 8, 0, // Skip to: 3452
/* 3444 */    MCD_OPC_CheckPredicate, 26, 38, 40, // Skip to: 13726
/* 3448 */    MCD_OPC_Decode, 242, 9, 114, // Opcode: NMADD_S
/* 3452 */    MCD_OPC_FilterValue, 49, 8, 0, // Skip to: 3464
/* 3456 */    MCD_OPC_CheckPredicate, 29, 26, 40, // Skip to: 13726
/* 3460 */    MCD_OPC_Decode, 239, 9, 115, // Opcode: NMADD_D32
/* 3464 */    MCD_OPC_FilterValue, 56, 8, 0, // Skip to: 3476
/* 3468 */    MCD_OPC_CheckPredicate, 26, 14, 40, // Skip to: 13726
/* 3472 */    MCD_OPC_Decode, 247, 9, 114, // Opcode: NMSUB_S
/* 3476 */    MCD_OPC_FilterValue, 57, 6, 40, // Skip to: 13726
/* 3480 */    MCD_OPC_CheckPredicate, 29, 2, 40, // Skip to: 13726
/* 3484 */    MCD_OPC_Decode, 244, 9, 115, // Opcode: NMSUB_D32
/* 3488 */    MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 3500
/* 3492 */    MCD_OPC_CheckPredicate, 16, 246, 39, // Skip to: 13726
/* 3496 */    MCD_OPC_Decode, 209, 1, 78, // Opcode: BEQL
/* 3500 */    MCD_OPC_FilterValue, 21, 8, 0, // Skip to: 3512
/* 3504 */    MCD_OPC_CheckPredicate, 16, 234, 39, // Skip to: 13726
/* 3508 */    MCD_OPC_Decode, 156, 2, 78, // Opcode: BNEL
/* 3512 */    MCD_OPC_FilterValue, 22, 14, 0, // Skip to: 3530
/* 3516 */    MCD_OPC_CheckPredicate, 16, 222, 39, // Skip to: 13726
/* 3520 */    MCD_OPC_CheckField, 16, 5, 0, 216, 39, // Skip to: 13726
/* 3526 */    MCD_OPC_Decode, 255, 1, 73, // Opcode: BLEZL
/* 3530 */    MCD_OPC_FilterValue, 23, 14, 0, // Skip to: 3548
/* 3534 */    MCD_OPC_CheckPredicate, 16, 204, 39, // Skip to: 13726
/* 3538 */    MCD_OPC_CheckField, 16, 5, 0, 198, 39, // Skip to: 13726
/* 3544 */    MCD_OPC_Decode, 231, 1, 73, // Opcode: BGTZL
/* 3548 */    MCD_OPC_FilterValue, 28, 229, 0, // Skip to: 3781
/* 3552 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 3555 */    MCD_OPC_FilterValue, 0, 36, 0, // Skip to: 3595
/* 3559 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3562 */    MCD_OPC_FilterValue, 0, 176, 39, // Skip to: 13726
/* 3566 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 3569 */    MCD_OPC_FilterValue, 0, 169, 39, // Skip to: 13726
/* 3573 */    MCD_OPC_CheckPredicate, 9, 10, 0, // Skip to: 3587
/* 3577 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 3587
/* 3583 */    MCD_OPC_Decode, 130, 8, 43, // Opcode: MADD
/* 3587 */    MCD_OPC_CheckPredicate, 12, 151, 39, // Skip to: 13726
/* 3591 */    MCD_OPC_Decode, 145, 8, 116, // Opcode: MADD_DSP
/* 3595 */    MCD_OPC_FilterValue, 1, 36, 0, // Skip to: 3635
/* 3599 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3602 */    MCD_OPC_FilterValue, 0, 136, 39, // Skip to: 13726
/* 3606 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 3609 */    MCD_OPC_FilterValue, 0, 129, 39, // Skip to: 13726
/* 3613 */    MCD_OPC_CheckPredicate, 9, 10, 0, // Skip to: 3627
/* 3617 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 3627
/* 3623 */    MCD_OPC_Decode, 135, 8, 43, // Opcode: MADDU
/* 3627 */    MCD_OPC_CheckPredicate, 12, 111, 39, // Skip to: 13726
/* 3631 */    MCD_OPC_Decode, 136, 8, 116, // Opcode: MADDU_DSP
/* 3635 */    MCD_OPC_FilterValue, 2, 14, 0, // Skip to: 3653
/* 3639 */    MCD_OPC_CheckPredicate, 9, 99, 39, // Skip to: 13726
/* 3643 */    MCD_OPC_CheckField, 6, 5, 0, 93, 39, // Skip to: 13726
/* 3649 */    MCD_OPC_Decode, 193, 9, 35, // Opcode: MUL
/* 3653 */    MCD_OPC_FilterValue, 4, 36, 0, // Skip to: 3693
/* 3657 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3660 */    MCD_OPC_FilterValue, 0, 78, 39, // Skip to: 13726
/* 3664 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 3667 */    MCD_OPC_FilterValue, 0, 71, 39, // Skip to: 13726
/* 3671 */    MCD_OPC_CheckPredicate, 9, 10, 0, // Skip to: 3685
/* 3675 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 3685
/* 3681 */    MCD_OPC_Decode, 148, 9, 43, // Opcode: MSUB
/* 3685 */    MCD_OPC_CheckPredicate, 12, 53, 39, // Skip to: 13726
/* 3689 */    MCD_OPC_Decode, 163, 9, 116, // Opcode: MSUB_DSP
/* 3693 */    MCD_OPC_FilterValue, 5, 36, 0, // Skip to: 3733
/* 3697 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3700 */    MCD_OPC_FilterValue, 0, 38, 39, // Skip to: 13726
/* 3704 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 3707 */    MCD_OPC_FilterValue, 0, 31, 39, // Skip to: 13726
/* 3711 */    MCD_OPC_CheckPredicate, 9, 10, 0, // Skip to: 3725
/* 3715 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 3725
/* 3721 */    MCD_OPC_Decode, 153, 9, 43, // Opcode: MSUBU
/* 3725 */    MCD_OPC_CheckPredicate, 12, 13, 39, // Skip to: 13726
/* 3729 */    MCD_OPC_Decode, 154, 9, 116, // Opcode: MSUBU_DSP
/* 3733 */    MCD_OPC_FilterValue, 32, 14, 0, // Skip to: 3751
/* 3737 */    MCD_OPC_CheckPredicate, 9, 1, 39, // Skip to: 13726
/* 3741 */    MCD_OPC_CheckField, 6, 5, 0, 251, 38, // Skip to: 13726
/* 3747 */    MCD_OPC_Decode, 152, 3, 117, // Opcode: CLZ
/* 3751 */    MCD_OPC_FilterValue, 33, 14, 0, // Skip to: 3769
/* 3755 */    MCD_OPC_CheckPredicate, 9, 239, 38, // Skip to: 13726
/* 3759 */    MCD_OPC_CheckField, 6, 5, 0, 233, 38, // Skip to: 13726
/* 3765 */    MCD_OPC_Decode, 133, 3, 117, // Opcode: CLO
/* 3769 */    MCD_OPC_FilterValue, 63, 225, 38, // Skip to: 13726
/* 3773 */    MCD_OPC_CheckPredicate, 9, 221, 38, // Skip to: 13726
/* 3777 */    MCD_OPC_Decode, 152, 11, 64, // Opcode: SDBBP
/* 3781 */    MCD_OPC_FilterValue, 29, 8, 0, // Skip to: 3793
/* 3785 */    MCD_OPC_CheckPredicate, 9, 209, 38, // Skip to: 13726
/* 3789 */    MCD_OPC_Decode, 131, 7, 77, // Opcode: JALX
/* 3793 */    MCD_OPC_FilterValue, 30, 28, 28, // Skip to: 10993
/* 3797 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 3800 */    MCD_OPC_FilterValue, 0, 50, 0, // Skip to: 3854
/* 3804 */    MCD_OPC_ExtractField, 24, 2,  // Inst{25-24} ...
/* 3807 */    MCD_OPC_FilterValue, 0, 7, 0, // Skip to: 3818
/* 3811 */    MCD_OPC_CheckPredicate, 8, 183, 38, // Skip to: 13726
/* 3815 */    MCD_OPC_Decode, 87, 118, // Opcode: ANDI_B
/* 3818 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 3830
/* 3822 */    MCD_OPC_CheckPredicate, 8, 172, 38, // Skip to: 13726
/* 3826 */    MCD_OPC_Decode, 136, 10, 118, // Opcode: ORI_B
/* 3830 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 3842
/* 3834 */    MCD_OPC_CheckPredicate, 8, 160, 38, // Skip to: 13726
/* 3838 */    MCD_OPC_Decode, 252, 9, 118, // Opcode: NORI_B
/* 3842 */    MCD_OPC_FilterValue, 3, 152, 38, // Skip to: 13726
/* 3846 */    MCD_OPC_CheckPredicate, 8, 148, 38, // Skip to: 13726
/* 3850 */    MCD_OPC_Decode, 239, 13, 118, // Opcode: XORI_B
/* 3854 */    MCD_OPC_FilterValue, 1, 39, 0, // Skip to: 3897
/* 3858 */    MCD_OPC_ExtractField, 24, 2,  // Inst{25-24} ...
/* 3861 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 3873
/* 3865 */    MCD_OPC_CheckPredicate, 8, 129, 38, // Skip to: 13726
/* 3869 */    MCD_OPC_Decode, 141, 2, 119, // Opcode: BMNZI_B
/* 3873 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 3885
/* 3877 */    MCD_OPC_CheckPredicate, 8, 117, 38, // Skip to: 13726
/* 3881 */    MCD_OPC_Decode, 143, 2, 119, // Opcode: BMZI_B
/* 3885 */    MCD_OPC_FilterValue, 2, 109, 38, // Skip to: 13726
/* 3889 */    MCD_OPC_CheckPredicate, 8, 105, 38, // Skip to: 13726
/* 3893 */    MCD_OPC_Decode, 174, 2, 119, // Opcode: BSELI_B
/* 3897 */    MCD_OPC_FilterValue, 2, 39, 0, // Skip to: 3940
/* 3901 */    MCD_OPC_ExtractField, 24, 2,  // Inst{25-24} ...
/* 3904 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 3916
/* 3908 */    MCD_OPC_CheckPredicate, 8, 86, 38, // Skip to: 13726
/* 3912 */    MCD_OPC_Decode, 189, 11, 118, // Opcode: SHF_B
/* 3916 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 3928
/* 3920 */    MCD_OPC_CheckPredicate, 8, 74, 38, // Skip to: 13726
/* 3924 */    MCD_OPC_Decode, 190, 11, 120, // Opcode: SHF_H
/* 3928 */    MCD_OPC_FilterValue, 2, 66, 38, // Skip to: 13726
/* 3932 */    MCD_OPC_CheckPredicate, 8, 62, 38, // Skip to: 13726
/* 3936 */    MCD_OPC_Decode, 191, 11, 121, // Opcode: SHF_W
/* 3940 */    MCD_OPC_FilterValue, 6, 31, 1, // Skip to: 4231
/* 3944 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 3947 */    MCD_OPC_FilterValue, 0, 7, 0, // Skip to: 3958
/* 3951 */    MCD_OPC_CheckPredicate, 8, 43, 38, // Skip to: 13726
/* 3955 */    MCD_OPC_Decode, 59, 122, // Opcode: ADDVI_B
/* 3958 */    MCD_OPC_FilterValue, 1, 7, 0, // Skip to: 3969
/* 3962 */    MCD_OPC_CheckPredicate, 8, 32, 38, // Skip to: 13726
/* 3966 */    MCD_OPC_Decode, 61, 123, // Opcode: ADDVI_H
/* 3969 */    MCD_OPC_FilterValue, 2, 7, 0, // Skip to: 3980
/* 3973 */    MCD_OPC_CheckPredicate, 8, 21, 38, // Skip to: 13726
/* 3977 */    MCD_OPC_Decode, 62, 124, // Opcode: ADDVI_W
/* 3980 */    MCD_OPC_FilterValue, 3, 7, 0, // Skip to: 3991
/* 3984 */    MCD_OPC_CheckPredicate, 8, 10, 38, // Skip to: 13726
/* 3988 */    MCD_OPC_Decode, 60, 125, // Opcode: ADDVI_D
/* 3991 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 4003
/* 3995 */    MCD_OPC_CheckPredicate, 8, 255, 37, // Skip to: 13726
/* 3999 */    MCD_OPC_Decode, 221, 12, 122, // Opcode: SUBVI_B
/* 4003 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 4015
/* 4007 */    MCD_OPC_CheckPredicate, 8, 243, 37, // Skip to: 13726
/* 4011 */    MCD_OPC_Decode, 223, 12, 123, // Opcode: SUBVI_H
/* 4015 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 4027
/* 4019 */    MCD_OPC_CheckPredicate, 8, 231, 37, // Skip to: 13726
/* 4023 */    MCD_OPC_Decode, 224, 12, 124, // Opcode: SUBVI_W
/* 4027 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 4039
/* 4031 */    MCD_OPC_CheckPredicate, 8, 219, 37, // Skip to: 13726
/* 4035 */    MCD_OPC_Decode, 222, 12, 125, // Opcode: SUBVI_D
/* 4039 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 4051
/* 4043 */    MCD_OPC_CheckPredicate, 8, 207, 37, // Skip to: 13726
/* 4047 */    MCD_OPC_Decode, 157, 8, 122, // Opcode: MAXI_S_B
/* 4051 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 4063
/* 4055 */    MCD_OPC_CheckPredicate, 8, 195, 37, // Skip to: 13726
/* 4059 */    MCD_OPC_Decode, 159, 8, 123, // Opcode: MAXI_S_H
/* 4063 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 4075
/* 4067 */    MCD_OPC_CheckPredicate, 8, 183, 37, // Skip to: 13726
/* 4071 */    MCD_OPC_Decode, 160, 8, 124, // Opcode: MAXI_S_W
/* 4075 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 4087
/* 4079 */    MCD_OPC_CheckPredicate, 8, 171, 37, // Skip to: 13726
/* 4083 */    MCD_OPC_Decode, 158, 8, 125, // Opcode: MAXI_S_D
/* 4087 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 4099
/* 4091 */    MCD_OPC_CheckPredicate, 8, 159, 37, // Skip to: 13726
/* 4095 */    MCD_OPC_Decode, 161, 8, 122, // Opcode: MAXI_U_B
/* 4099 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 4111
/* 4103 */    MCD_OPC_CheckPredicate, 8, 147, 37, // Skip to: 13726
/* 4107 */    MCD_OPC_Decode, 163, 8, 123, // Opcode: MAXI_U_H
/* 4111 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 4123
/* 4115 */    MCD_OPC_CheckPredicate, 8, 135, 37, // Skip to: 13726
/* 4119 */    MCD_OPC_Decode, 164, 8, 124, // Opcode: MAXI_U_W
/* 4123 */    MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 4135
/* 4127 */    MCD_OPC_CheckPredicate, 8, 123, 37, // Skip to: 13726
/* 4131 */    MCD_OPC_Decode, 162, 8, 125, // Opcode: MAXI_U_D
/* 4135 */    MCD_OPC_FilterValue, 16, 8, 0, // Skip to: 4147
/* 4139 */    MCD_OPC_CheckPredicate, 8, 111, 37, // Skip to: 13726
/* 4143 */    MCD_OPC_Decode, 198, 8, 122, // Opcode: MINI_S_B
/* 4147 */    MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 4159
/* 4151 */    MCD_OPC_CheckPredicate, 8, 99, 37, // Skip to: 13726
/* 4155 */    MCD_OPC_Decode, 200, 8, 123, // Opcode: MINI_S_H
/* 4159 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 4171
/* 4163 */    MCD_OPC_CheckPredicate, 8, 87, 37, // Skip to: 13726
/* 4167 */    MCD_OPC_Decode, 201, 8, 124, // Opcode: MINI_S_W
/* 4171 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 4183
/* 4175 */    MCD_OPC_CheckPredicate, 8, 75, 37, // Skip to: 13726
/* 4179 */    MCD_OPC_Decode, 199, 8, 125, // Opcode: MINI_S_D
/* 4183 */    MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 4195
/* 4187 */    MCD_OPC_CheckPredicate, 8, 63, 37, // Skip to: 13726
/* 4191 */    MCD_OPC_Decode, 202, 8, 122, // Opcode: MINI_U_B
/* 4195 */    MCD_OPC_FilterValue, 21, 8, 0, // Skip to: 4207
/* 4199 */    MCD_OPC_CheckPredicate, 8, 51, 37, // Skip to: 13726
/* 4203 */    MCD_OPC_Decode, 204, 8, 123, // Opcode: MINI_U_H
/* 4207 */    MCD_OPC_FilterValue, 22, 8, 0, // Skip to: 4219
/* 4211 */    MCD_OPC_CheckPredicate, 8, 39, 37, // Skip to: 13726
/* 4215 */    MCD_OPC_Decode, 205, 8, 124, // Opcode: MINI_U_W
/* 4219 */    MCD_OPC_FilterValue, 23, 31, 37, // Skip to: 13726
/* 4223 */    MCD_OPC_CheckPredicate, 8, 27, 37, // Skip to: 13726
/* 4227 */    MCD_OPC_Decode, 203, 8, 125, // Opcode: MINI_U_D
/* 4231 */    MCD_OPC_FilterValue, 7, 37, 1, // Skip to: 4528
/* 4235 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 4238 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4250
/* 4242 */    MCD_OPC_CheckPredicate, 8, 8, 37, // Skip to: 13726
/* 4246 */    MCD_OPC_Decode, 230, 2, 122, // Opcode: CEQI_B
/* 4250 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 4262
/* 4254 */    MCD_OPC_CheckPredicate, 8, 252, 36, // Skip to: 13726
/* 4258 */    MCD_OPC_Decode, 232, 2, 123, // Opcode: CEQI_H
/* 4262 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 4274
/* 4266 */    MCD_OPC_CheckPredicate, 8, 240, 36, // Skip to: 13726
/* 4270 */    MCD_OPC_Decode, 233, 2, 124, // Opcode: CEQI_W
/* 4274 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 4286
/* 4278 */    MCD_OPC_CheckPredicate, 8, 228, 36, // Skip to: 13726
/* 4282 */    MCD_OPC_Decode, 231, 2, 125, // Opcode: CEQI_D
/* 4286 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 4298
/* 4290 */    MCD_OPC_CheckPredicate, 8, 216, 36, // Skip to: 13726
/* 4294 */    MCD_OPC_Decode, 136, 3, 122, // Opcode: CLTI_S_B
/* 4298 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 4310
/* 4302 */    MCD_OPC_CheckPredicate, 8, 204, 36, // Skip to: 13726
/* 4306 */    MCD_OPC_Decode, 138, 3, 123, // Opcode: CLTI_S_H
/* 4310 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 4322
/* 4314 */    MCD_OPC_CheckPredicate, 8, 192, 36, // Skip to: 13726
/* 4318 */    MCD_OPC_Decode, 139, 3, 124, // Opcode: CLTI_S_W
/* 4322 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 4334
/* 4326 */    MCD_OPC_CheckPredicate, 8, 180, 36, // Skip to: 13726
/* 4330 */    MCD_OPC_Decode, 137, 3, 125, // Opcode: CLTI_S_D
/* 4334 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 4346
/* 4338 */    MCD_OPC_CheckPredicate, 8, 168, 36, // Skip to: 13726
/* 4342 */    MCD_OPC_Decode, 140, 3, 122, // Opcode: CLTI_U_B
/* 4346 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 4358
/* 4350 */    MCD_OPC_CheckPredicate, 8, 156, 36, // Skip to: 13726
/* 4354 */    MCD_OPC_Decode, 142, 3, 123, // Opcode: CLTI_U_H
/* 4358 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 4370
/* 4362 */    MCD_OPC_CheckPredicate, 8, 144, 36, // Skip to: 13726
/* 4366 */    MCD_OPC_Decode, 143, 3, 124, // Opcode: CLTI_U_W
/* 4370 */    MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 4382
/* 4374 */    MCD_OPC_CheckPredicate, 8, 132, 36, // Skip to: 13726
/* 4378 */    MCD_OPC_Decode, 141, 3, 125, // Opcode: CLTI_U_D
/* 4382 */    MCD_OPC_FilterValue, 16, 8, 0, // Skip to: 4394
/* 4386 */    MCD_OPC_CheckPredicate, 8, 120, 36, // Skip to: 13726
/* 4390 */    MCD_OPC_Decode, 245, 2, 122, // Opcode: CLEI_S_B
/* 4394 */    MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 4406
/* 4398 */    MCD_OPC_CheckPredicate, 8, 108, 36, // Skip to: 13726
/* 4402 */    MCD_OPC_Decode, 247, 2, 123, // Opcode: CLEI_S_H
/* 4406 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 4418
/* 4410 */    MCD_OPC_CheckPredicate, 8, 96, 36, // Skip to: 13726
/* 4414 */    MCD_OPC_Decode, 248, 2, 124, // Opcode: CLEI_S_W
/* 4418 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 4430
/* 4422 */    MCD_OPC_CheckPredicate, 8, 84, 36, // Skip to: 13726
/* 4426 */    MCD_OPC_Decode, 246, 2, 125, // Opcode: CLEI_S_D
/* 4430 */    MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 4442
/* 4434 */    MCD_OPC_CheckPredicate, 8, 72, 36, // Skip to: 13726
/* 4438 */    MCD_OPC_Decode, 249, 2, 122, // Opcode: CLEI_U_B
/* 4442 */    MCD_OPC_FilterValue, 21, 8, 0, // Skip to: 4454
/* 4446 */    MCD_OPC_CheckPredicate, 8, 60, 36, // Skip to: 13726
/* 4450 */    MCD_OPC_Decode, 251, 2, 123, // Opcode: CLEI_U_H
/* 4454 */    MCD_OPC_FilterValue, 22, 8, 0, // Skip to: 4466
/* 4458 */    MCD_OPC_CheckPredicate, 8, 48, 36, // Skip to: 13726
/* 4462 */    MCD_OPC_Decode, 252, 2, 124, // Opcode: CLEI_U_W
/* 4466 */    MCD_OPC_FilterValue, 23, 8, 0, // Skip to: 4478
/* 4470 */    MCD_OPC_CheckPredicate, 8, 36, 36, // Skip to: 13726
/* 4474 */    MCD_OPC_Decode, 250, 2, 125, // Opcode: CLEI_U_D
/* 4478 */    MCD_OPC_FilterValue, 24, 8, 0, // Skip to: 4490
/* 4482 */    MCD_OPC_CheckPredicate, 8, 24, 36, // Skip to: 13726
/* 4486 */    MCD_OPC_Decode, 168, 7, 126, // Opcode: LDI_B
/* 4490 */    MCD_OPC_FilterValue, 25, 8, 0, // Skip to: 4502
/* 4494 */    MCD_OPC_CheckPredicate, 8, 12, 36, // Skip to: 13726
/* 4498 */    MCD_OPC_Decode, 170, 7, 127, // Opcode: LDI_H
/* 4502 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 4515
/* 4506 */    MCD_OPC_CheckPredicate, 8, 0, 36, // Skip to: 13726
/* 4510 */    MCD_OPC_Decode, 171, 7, 128, 1, // Opcode: LDI_W
/* 4515 */    MCD_OPC_FilterValue, 27, 247, 35, // Skip to: 13726
/* 4519 */    MCD_OPC_CheckPredicate, 8, 243, 35, // Skip to: 13726
/* 4523 */    MCD_OPC_Decode, 169, 7, 129, 1, // Opcode: LDI_D
/* 4528 */    MCD_OPC_FilterValue, 9, 61, 2, // Skip to: 5105
/* 4532 */    MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 4535 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4548
/* 4539 */    MCD_OPC_CheckPredicate, 8, 223, 35, // Skip to: 13726
/* 4543 */    MCD_OPC_Decode, 230, 11, 130, 1, // Opcode: SLLI_D
/* 4548 */    MCD_OPC_FilterValue, 1, 54, 0, // Skip to: 4606
/* 4552 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4555 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4567
/* 4559 */    MCD_OPC_CheckPredicate, 8, 203, 35, // Skip to: 13726
/* 4563 */    MCD_OPC_Decode, 232, 11, 124, // Opcode: SLLI_W
/* 4567 */    MCD_OPC_FilterValue, 1, 195, 35, // Skip to: 13726
/* 4571 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4574 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4587
/* 4578 */    MCD_OPC_CheckPredicate, 8, 184, 35, // Skip to: 13726
/* 4582 */    MCD_OPC_Decode, 231, 11, 131, 1, // Opcode: SLLI_H
/* 4587 */    MCD_OPC_FilterValue, 1, 175, 35, // Skip to: 13726
/* 4591 */    MCD_OPC_CheckPredicate, 8, 171, 35, // Skip to: 13726
/* 4595 */    MCD_OPC_CheckField, 19, 1, 0, 165, 35, // Skip to: 13726
/* 4601 */    MCD_OPC_Decode, 229, 11, 132, 1, // Opcode: SLLI_B
/* 4606 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 4619
/* 4610 */    MCD_OPC_CheckPredicate, 8, 152, 35, // Skip to: 13726
/* 4614 */    MCD_OPC_Decode, 141, 12, 130, 1, // Opcode: SRAI_D
/* 4619 */    MCD_OPC_FilterValue, 3, 54, 0, // Skip to: 4677
/* 4623 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4626 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4638
/* 4630 */    MCD_OPC_CheckPredicate, 8, 132, 35, // Skip to: 13726
/* 4634 */    MCD_OPC_Decode, 143, 12, 124, // Opcode: SRAI_W
/* 4638 */    MCD_OPC_FilterValue, 1, 124, 35, // Skip to: 13726
/* 4642 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4645 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4658
/* 4649 */    MCD_OPC_CheckPredicate, 8, 113, 35, // Skip to: 13726
/* 4653 */    MCD_OPC_Decode, 142, 12, 131, 1, // Opcode: SRAI_H
/* 4658 */    MCD_OPC_FilterValue, 1, 104, 35, // Skip to: 13726
/* 4662 */    MCD_OPC_CheckPredicate, 8, 100, 35, // Skip to: 13726
/* 4666 */    MCD_OPC_CheckField, 19, 1, 0, 94, 35, // Skip to: 13726
/* 4672 */    MCD_OPC_Decode, 140, 12, 132, 1, // Opcode: SRAI_B
/* 4677 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 4690
/* 4681 */    MCD_OPC_CheckPredicate, 8, 81, 35, // Skip to: 13726
/* 4685 */    MCD_OPC_Decode, 162, 12, 130, 1, // Opcode: SRLI_D
/* 4690 */    MCD_OPC_FilterValue, 5, 54, 0, // Skip to: 4748
/* 4694 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4697 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4709
/* 4701 */    MCD_OPC_CheckPredicate, 8, 61, 35, // Skip to: 13726
/* 4705 */    MCD_OPC_Decode, 164, 12, 124, // Opcode: SRLI_W
/* 4709 */    MCD_OPC_FilterValue, 1, 53, 35, // Skip to: 13726
/* 4713 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4716 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4729
/* 4720 */    MCD_OPC_CheckPredicate, 8, 42, 35, // Skip to: 13726
/* 4724 */    MCD_OPC_Decode, 163, 12, 131, 1, // Opcode: SRLI_H
/* 4729 */    MCD_OPC_FilterValue, 1, 33, 35, // Skip to: 13726
/* 4733 */    MCD_OPC_CheckPredicate, 8, 29, 35, // Skip to: 13726
/* 4737 */    MCD_OPC_CheckField, 19, 1, 0, 23, 35, // Skip to: 13726
/* 4743 */    MCD_OPC_Decode, 161, 12, 132, 1, // Opcode: SRLI_B
/* 4748 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 4761
/* 4752 */    MCD_OPC_CheckPredicate, 8, 10, 35, // Skip to: 13726
/* 4756 */    MCD_OPC_Decode, 199, 1, 130, 1, // Opcode: BCLRI_D
/* 4761 */    MCD_OPC_FilterValue, 7, 54, 0, // Skip to: 4819
/* 4765 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4768 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4780
/* 4772 */    MCD_OPC_CheckPredicate, 8, 246, 34, // Skip to: 13726
/* 4776 */    MCD_OPC_Decode, 201, 1, 124, // Opcode: BCLRI_W
/* 4780 */    MCD_OPC_FilterValue, 1, 238, 34, // Skip to: 13726
/* 4784 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4787 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4800
/* 4791 */    MCD_OPC_CheckPredicate, 8, 227, 34, // Skip to: 13726
/* 4795 */    MCD_OPC_Decode, 200, 1, 131, 1, // Opcode: BCLRI_H
/* 4800 */    MCD_OPC_FilterValue, 1, 218, 34, // Skip to: 13726
/* 4804 */    MCD_OPC_CheckPredicate, 8, 214, 34, // Skip to: 13726
/* 4808 */    MCD_OPC_CheckField, 19, 1, 0, 208, 34, // Skip to: 13726
/* 4814 */    MCD_OPC_Decode, 198, 1, 132, 1, // Opcode: BCLRI_B
/* 4819 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 4832
/* 4823 */    MCD_OPC_CheckPredicate, 8, 195, 34, // Skip to: 13726
/* 4827 */    MCD_OPC_Decode, 182, 2, 130, 1, // Opcode: BSETI_D
/* 4832 */    MCD_OPC_FilterValue, 9, 54, 0, // Skip to: 4890
/* 4836 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4839 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4851
/* 4843 */    MCD_OPC_CheckPredicate, 8, 175, 34, // Skip to: 13726
/* 4847 */    MCD_OPC_Decode, 184, 2, 124, // Opcode: BSETI_W
/* 4851 */    MCD_OPC_FilterValue, 1, 167, 34, // Skip to: 13726
/* 4855 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4858 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4871
/* 4862 */    MCD_OPC_CheckPredicate, 8, 156, 34, // Skip to: 13726
/* 4866 */    MCD_OPC_Decode, 183, 2, 131, 1, // Opcode: BSETI_H
/* 4871 */    MCD_OPC_FilterValue, 1, 147, 34, // Skip to: 13726
/* 4875 */    MCD_OPC_CheckPredicate, 8, 143, 34, // Skip to: 13726
/* 4879 */    MCD_OPC_CheckField, 19, 1, 0, 137, 34, // Skip to: 13726
/* 4885 */    MCD_OPC_Decode, 181, 2, 132, 1, // Opcode: BSETI_B
/* 4890 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 4903
/* 4894 */    MCD_OPC_CheckPredicate, 8, 124, 34, // Skip to: 13726
/* 4898 */    MCD_OPC_Decode, 149, 2, 130, 1, // Opcode: BNEGI_D
/* 4903 */    MCD_OPC_FilterValue, 11, 54, 0, // Skip to: 4961
/* 4907 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4910 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 4922
/* 4914 */    MCD_OPC_CheckPredicate, 8, 104, 34, // Skip to: 13726
/* 4918 */    MCD_OPC_Decode, 151, 2, 124, // Opcode: BNEGI_W
/* 4922 */    MCD_OPC_FilterValue, 1, 96, 34, // Skip to: 13726
/* 4926 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4929 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4942
/* 4933 */    MCD_OPC_CheckPredicate, 8, 85, 34, // Skip to: 13726
/* 4937 */    MCD_OPC_Decode, 150, 2, 131, 1, // Opcode: BNEGI_H
/* 4942 */    MCD_OPC_FilterValue, 1, 76, 34, // Skip to: 13726
/* 4946 */    MCD_OPC_CheckPredicate, 8, 72, 34, // Skip to: 13726
/* 4950 */    MCD_OPC_CheckField, 19, 1, 0, 66, 34, // Skip to: 13726
/* 4956 */    MCD_OPC_Decode, 148, 2, 132, 1, // Opcode: BNEGI_B
/* 4961 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 4974
/* 4965 */    MCD_OPC_CheckPredicate, 8, 53, 34, // Skip to: 13726
/* 4969 */    MCD_OPC_Decode, 234, 1, 133, 1, // Opcode: BINSLI_D
/* 4974 */    MCD_OPC_FilterValue, 13, 55, 0, // Skip to: 5033
/* 4978 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 4981 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 4994
/* 4985 */    MCD_OPC_CheckPredicate, 8, 33, 34, // Skip to: 13726
/* 4989 */    MCD_OPC_Decode, 236, 1, 134, 1, // Opcode: BINSLI_W
/* 4994 */    MCD_OPC_FilterValue, 1, 24, 34, // Skip to: 13726
/* 4998 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5001 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5014
/* 5005 */    MCD_OPC_CheckPredicate, 8, 13, 34, // Skip to: 13726
/* 5009 */    MCD_OPC_Decode, 235, 1, 135, 1, // Opcode: BINSLI_H
/* 5014 */    MCD_OPC_FilterValue, 1, 4, 34, // Skip to: 13726
/* 5018 */    MCD_OPC_CheckPredicate, 8, 0, 34, // Skip to: 13726
/* 5022 */    MCD_OPC_CheckField, 19, 1, 0, 250, 33, // Skip to: 13726
/* 5028 */    MCD_OPC_Decode, 233, 1, 136, 1, // Opcode: BINSLI_B
/* 5033 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 5046
/* 5037 */    MCD_OPC_CheckPredicate, 8, 237, 33, // Skip to: 13726
/* 5041 */    MCD_OPC_Decode, 242, 1, 133, 1, // Opcode: BINSRI_D
/* 5046 */    MCD_OPC_FilterValue, 15, 228, 33, // Skip to: 13726
/* 5050 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 5053 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5066
/* 5057 */    MCD_OPC_CheckPredicate, 8, 217, 33, // Skip to: 13726
/* 5061 */    MCD_OPC_Decode, 244, 1, 134, 1, // Opcode: BINSRI_W
/* 5066 */    MCD_OPC_FilterValue, 1, 208, 33, // Skip to: 13726
/* 5070 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5073 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5086
/* 5077 */    MCD_OPC_CheckPredicate, 8, 197, 33, // Skip to: 13726
/* 5081 */    MCD_OPC_Decode, 243, 1, 135, 1, // Opcode: BINSRI_H
/* 5086 */    MCD_OPC_FilterValue, 1, 188, 33, // Skip to: 13726
/* 5090 */    MCD_OPC_CheckPredicate, 8, 184, 33, // Skip to: 13726
/* 5094 */    MCD_OPC_CheckField, 19, 1, 0, 178, 33, // Skip to: 13726
/* 5100 */    MCD_OPC_Decode, 241, 1, 136, 1, // Opcode: BINSRI_B
/* 5105 */    MCD_OPC_FilterValue, 10, 31, 1, // Skip to: 5396
/* 5109 */    MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 5112 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5125
/* 5116 */    MCD_OPC_CheckPredicate, 8, 158, 33, // Skip to: 13726
/* 5120 */    MCD_OPC_Decode, 135, 11, 130, 1, // Opcode: SAT_S_D
/* 5125 */    MCD_OPC_FilterValue, 1, 54, 0, // Skip to: 5183
/* 5129 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 5132 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 5144
/* 5136 */    MCD_OPC_CheckPredicate, 8, 138, 33, // Skip to: 13726
/* 5140 */    MCD_OPC_Decode, 137, 11, 124, // Opcode: SAT_S_W
/* 5144 */    MCD_OPC_FilterValue, 1, 130, 33, // Skip to: 13726
/* 5148 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5151 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5164
/* 5155 */    MCD_OPC_CheckPredicate, 8, 119, 33, // Skip to: 13726
/* 5159 */    MCD_OPC_Decode, 136, 11, 131, 1, // Opcode: SAT_S_H
/* 5164 */    MCD_OPC_FilterValue, 1, 110, 33, // Skip to: 13726
/* 5168 */    MCD_OPC_CheckPredicate, 8, 106, 33, // Skip to: 13726
/* 5172 */    MCD_OPC_CheckField, 19, 1, 0, 100, 33, // Skip to: 13726
/* 5178 */    MCD_OPC_Decode, 134, 11, 132, 1, // Opcode: SAT_S_B
/* 5183 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 5196
/* 5187 */    MCD_OPC_CheckPredicate, 8, 87, 33, // Skip to: 13726
/* 5191 */    MCD_OPC_Decode, 139, 11, 130, 1, // Opcode: SAT_U_D
/* 5196 */    MCD_OPC_FilterValue, 3, 54, 0, // Skip to: 5254
/* 5200 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 5203 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 5215
/* 5207 */    MCD_OPC_CheckPredicate, 8, 67, 33, // Skip to: 13726
/* 5211 */    MCD_OPC_Decode, 141, 11, 124, // Opcode: SAT_U_W
/* 5215 */    MCD_OPC_FilterValue, 1, 59, 33, // Skip to: 13726
/* 5219 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5222 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5235
/* 5226 */    MCD_OPC_CheckPredicate, 8, 48, 33, // Skip to: 13726
/* 5230 */    MCD_OPC_Decode, 140, 11, 131, 1, // Opcode: SAT_U_H
/* 5235 */    MCD_OPC_FilterValue, 1, 39, 33, // Skip to: 13726
/* 5239 */    MCD_OPC_CheckPredicate, 8, 35, 33, // Skip to: 13726
/* 5243 */    MCD_OPC_CheckField, 19, 1, 0, 29, 33, // Skip to: 13726
/* 5249 */    MCD_OPC_Decode, 138, 11, 132, 1, // Opcode: SAT_U_B
/* 5254 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 5267
/* 5258 */    MCD_OPC_CheckPredicate, 8, 16, 33, // Skip to: 13726
/* 5262 */    MCD_OPC_Decode, 145, 12, 130, 1, // Opcode: SRARI_D
/* 5267 */    MCD_OPC_FilterValue, 5, 54, 0, // Skip to: 5325
/* 5271 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 5274 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 5286
/* 5278 */    MCD_OPC_CheckPredicate, 8, 252, 32, // Skip to: 13726
/* 5282 */    MCD_OPC_Decode, 147, 12, 124, // Opcode: SRARI_W
/* 5286 */    MCD_OPC_FilterValue, 1, 244, 32, // Skip to: 13726
/* 5290 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5293 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5306
/* 5297 */    MCD_OPC_CheckPredicate, 8, 233, 32, // Skip to: 13726
/* 5301 */    MCD_OPC_Decode, 146, 12, 131, 1, // Opcode: SRARI_H
/* 5306 */    MCD_OPC_FilterValue, 1, 224, 32, // Skip to: 13726
/* 5310 */    MCD_OPC_CheckPredicate, 8, 220, 32, // Skip to: 13726
/* 5314 */    MCD_OPC_CheckField, 19, 1, 0, 214, 32, // Skip to: 13726
/* 5320 */    MCD_OPC_Decode, 144, 12, 132, 1, // Opcode: SRARI_B
/* 5325 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 5338
/* 5329 */    MCD_OPC_CheckPredicate, 8, 201, 32, // Skip to: 13726
/* 5333 */    MCD_OPC_Decode, 166, 12, 130, 1, // Opcode: SRLRI_D
/* 5338 */    MCD_OPC_FilterValue, 7, 192, 32, // Skip to: 13726
/* 5342 */    MCD_OPC_ExtractField, 21, 1,  // Inst{21} ...
/* 5345 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 5357
/* 5349 */    MCD_OPC_CheckPredicate, 8, 181, 32, // Skip to: 13726
/* 5353 */    MCD_OPC_Decode, 168, 12, 124, // Opcode: SRLRI_W
/* 5357 */    MCD_OPC_FilterValue, 1, 173, 32, // Skip to: 13726
/* 5361 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 5364 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5377
/* 5368 */    MCD_OPC_CheckPredicate, 8, 162, 32, // Skip to: 13726
/* 5372 */    MCD_OPC_Decode, 167, 12, 131, 1, // Opcode: SRLRI_H
/* 5377 */    MCD_OPC_FilterValue, 1, 153, 32, // Skip to: 13726
/* 5381 */    MCD_OPC_CheckPredicate, 8, 149, 32, // Skip to: 13726
/* 5385 */    MCD_OPC_CheckField, 19, 1, 0, 143, 32, // Skip to: 13726
/* 5391 */    MCD_OPC_Decode, 165, 12, 132, 1, // Opcode: SRLRI_B
/* 5396 */    MCD_OPC_FilterValue, 13, 163, 1, // Skip to: 5819
/* 5400 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 5403 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 5416
/* 5407 */    MCD_OPC_CheckPredicate, 8, 123, 32, // Skip to: 13726
/* 5411 */    MCD_OPC_Decode, 235, 11, 137, 1, // Opcode: SLL_B
/* 5416 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 5429
/* 5420 */    MCD_OPC_CheckPredicate, 8, 110, 32, // Skip to: 13726
/* 5424 */    MCD_OPC_Decode, 237, 11, 138, 1, // Opcode: SLL_H
/* 5429 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 5442
/* 5433 */    MCD_OPC_CheckPredicate, 8, 97, 32, // Skip to: 13726
/* 5437 */    MCD_OPC_Decode, 239, 11, 139, 1, // Opcode: SLL_W
/* 5442 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 5455
/* 5446 */    MCD_OPC_CheckPredicate, 8, 84, 32, // Skip to: 13726
/* 5450 */    MCD_OPC_Decode, 236, 11, 140, 1, // Opcode: SLL_D
/* 5455 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 5468
/* 5459 */    MCD_OPC_CheckPredicate, 8, 71, 32, // Skip to: 13726
/* 5463 */    MCD_OPC_Decode, 154, 12, 137, 1, // Opcode: SRA_B
/* 5468 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 5481
/* 5472 */    MCD_OPC_CheckPredicate, 8, 58, 32, // Skip to: 13726
/* 5476 */    MCD_OPC_Decode, 156, 12, 138, 1, // Opcode: SRA_H
/* 5481 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 5494
/* 5485 */    MCD_OPC_CheckPredicate, 8, 45, 32, // Skip to: 13726
/* 5489 */    MCD_OPC_Decode, 158, 12, 139, 1, // Opcode: SRA_W
/* 5494 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 5507
/* 5498 */    MCD_OPC_CheckPredicate, 8, 32, 32, // Skip to: 13726
/* 5502 */    MCD_OPC_Decode, 155, 12, 140, 1, // Opcode: SRA_D
/* 5507 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 5520
/* 5511 */    MCD_OPC_CheckPredicate, 8, 19, 32, // Skip to: 13726
/* 5515 */    MCD_OPC_Decode, 175, 12, 137, 1, // Opcode: SRL_B
/* 5520 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 5533
/* 5524 */    MCD_OPC_CheckPredicate, 8, 6, 32, // Skip to: 13726
/* 5528 */    MCD_OPC_Decode, 177, 12, 138, 1, // Opcode: SRL_H
/* 5533 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 5546
/* 5537 */    MCD_OPC_CheckPredicate, 8, 249, 31, // Skip to: 13726
/* 5541 */    MCD_OPC_Decode, 179, 12, 139, 1, // Opcode: SRL_W
/* 5546 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 5559
/* 5550 */    MCD_OPC_CheckPredicate, 8, 236, 31, // Skip to: 13726
/* 5554 */    MCD_OPC_Decode, 176, 12, 140, 1, // Opcode: SRL_D
/* 5559 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 5572
/* 5563 */    MCD_OPC_CheckPredicate, 8, 223, 31, // Skip to: 13726
/* 5567 */    MCD_OPC_Decode, 202, 1, 137, 1, // Opcode: BCLR_B
/* 5572 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 5585
/* 5576 */    MCD_OPC_CheckPredicate, 8, 210, 31, // Skip to: 13726
/* 5580 */    MCD_OPC_Decode, 204, 1, 138, 1, // Opcode: BCLR_H
/* 5585 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 5598
/* 5589 */    MCD_OPC_CheckPredicate, 8, 197, 31, // Skip to: 13726
/* 5593 */    MCD_OPC_Decode, 205, 1, 139, 1, // Opcode: BCLR_W
/* 5598 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 5611
/* 5602 */    MCD_OPC_CheckPredicate, 8, 184, 31, // Skip to: 13726
/* 5606 */    MCD_OPC_Decode, 203, 1, 140, 1, // Opcode: BCLR_D
/* 5611 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 5624
/* 5615 */    MCD_OPC_CheckPredicate, 8, 171, 31, // Skip to: 13726
/* 5619 */    MCD_OPC_Decode, 185, 2, 137, 1, // Opcode: BSET_B
/* 5624 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 5637
/* 5628 */    MCD_OPC_CheckPredicate, 8, 158, 31, // Skip to: 13726
/* 5632 */    MCD_OPC_Decode, 187, 2, 138, 1, // Opcode: BSET_H
/* 5637 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 5650
/* 5641 */    MCD_OPC_CheckPredicate, 8, 145, 31, // Skip to: 13726
/* 5645 */    MCD_OPC_Decode, 188, 2, 139, 1, // Opcode: BSET_W
/* 5650 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 5663
/* 5654 */    MCD_OPC_CheckPredicate, 8, 132, 31, // Skip to: 13726
/* 5658 */    MCD_OPC_Decode, 186, 2, 140, 1, // Opcode: BSET_D
/* 5663 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 5676
/* 5667 */    MCD_OPC_CheckPredicate, 8, 119, 31, // Skip to: 13726
/* 5671 */    MCD_OPC_Decode, 152, 2, 137, 1, // Opcode: BNEG_B
/* 5676 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 5689
/* 5680 */    MCD_OPC_CheckPredicate, 8, 106, 31, // Skip to: 13726
/* 5684 */    MCD_OPC_Decode, 154, 2, 138, 1, // Opcode: BNEG_H
/* 5689 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 5702
/* 5693 */    MCD_OPC_CheckPredicate, 8, 93, 31, // Skip to: 13726
/* 5697 */    MCD_OPC_Decode, 155, 2, 139, 1, // Opcode: BNEG_W
/* 5702 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 5715
/* 5706 */    MCD_OPC_CheckPredicate, 8, 80, 31, // Skip to: 13726
/* 5710 */    MCD_OPC_Decode, 153, 2, 140, 1, // Opcode: BNEG_D
/* 5715 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 5728
/* 5719 */    MCD_OPC_CheckPredicate, 8, 67, 31, // Skip to: 13726
/* 5723 */    MCD_OPC_Decode, 237, 1, 141, 1, // Opcode: BINSL_B
/* 5728 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 5741
/* 5732 */    MCD_OPC_CheckPredicate, 8, 54, 31, // Skip to: 13726
/* 5736 */    MCD_OPC_Decode, 239, 1, 142, 1, // Opcode: BINSL_H
/* 5741 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 5754
/* 5745 */    MCD_OPC_CheckPredicate, 8, 41, 31, // Skip to: 13726
/* 5749 */    MCD_OPC_Decode, 240, 1, 143, 1, // Opcode: BINSL_W
/* 5754 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 5767
/* 5758 */    MCD_OPC_CheckPredicate, 8, 28, 31, // Skip to: 13726
/* 5762 */    MCD_OPC_Decode, 238, 1, 144, 1, // Opcode: BINSL_D
/* 5767 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 5780
/* 5771 */    MCD_OPC_CheckPredicate, 8, 15, 31, // Skip to: 13726
/* 5775 */    MCD_OPC_Decode, 245, 1, 141, 1, // Opcode: BINSR_B
/* 5780 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 5793
/* 5784 */    MCD_OPC_CheckPredicate, 8, 2, 31, // Skip to: 13726
/* 5788 */    MCD_OPC_Decode, 247, 1, 142, 1, // Opcode: BINSR_H
/* 5793 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 5806
/* 5797 */    MCD_OPC_CheckPredicate, 8, 245, 30, // Skip to: 13726
/* 5801 */    MCD_OPC_Decode, 248, 1, 143, 1, // Opcode: BINSR_W
/* 5806 */    MCD_OPC_FilterValue, 31, 236, 30, // Skip to: 13726
/* 5810 */    MCD_OPC_CheckPredicate, 8, 232, 30, // Skip to: 13726
/* 5814 */    MCD_OPC_Decode, 246, 1, 144, 1, // Opcode: BINSR_D
/* 5819 */    MCD_OPC_FilterValue, 14, 159, 1, // Skip to: 6238
/* 5823 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 5826 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 5838
/* 5830 */    MCD_OPC_CheckPredicate, 8, 212, 30, // Skip to: 13726
/* 5834 */    MCD_OPC_Decode, 63, 137, 1, // Opcode: ADDV_B
/* 5838 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 5850
/* 5842 */    MCD_OPC_CheckPredicate, 8, 200, 30, // Skip to: 13726
/* 5846 */    MCD_OPC_Decode, 65, 138, 1, // Opcode: ADDV_H
/* 5850 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 5862
/* 5854 */    MCD_OPC_CheckPredicate, 8, 188, 30, // Skip to: 13726
/* 5858 */    MCD_OPC_Decode, 66, 139, 1, // Opcode: ADDV_W
/* 5862 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 5874
/* 5866 */    MCD_OPC_CheckPredicate, 8, 176, 30, // Skip to: 13726
/* 5870 */    MCD_OPC_Decode, 64, 140, 1, // Opcode: ADDV_D
/* 5874 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 5887
/* 5878 */    MCD_OPC_CheckPredicate, 8, 164, 30, // Skip to: 13726
/* 5882 */    MCD_OPC_Decode, 225, 12, 137, 1, // Opcode: SUBV_B
/* 5887 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 5900
/* 5891 */    MCD_OPC_CheckPredicate, 8, 151, 30, // Skip to: 13726
/* 5895 */    MCD_OPC_Decode, 227, 12, 138, 1, // Opcode: SUBV_H
/* 5900 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 5913
/* 5904 */    MCD_OPC_CheckPredicate, 8, 138, 30, // Skip to: 13726
/* 5908 */    MCD_OPC_Decode, 228, 12, 139, 1, // Opcode: SUBV_W
/* 5913 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 5926
/* 5917 */    MCD_OPC_CheckPredicate, 8, 125, 30, // Skip to: 13726
/* 5921 */    MCD_OPC_Decode, 226, 12, 140, 1, // Opcode: SUBV_D
/* 5926 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 5939
/* 5930 */    MCD_OPC_CheckPredicate, 8, 112, 30, // Skip to: 13726
/* 5934 */    MCD_OPC_Decode, 171, 8, 137, 1, // Opcode: MAX_S_B
/* 5939 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 5952
/* 5943 */    MCD_OPC_CheckPredicate, 8, 99, 30, // Skip to: 13726
/* 5947 */    MCD_OPC_Decode, 173, 8, 138, 1, // Opcode: MAX_S_H
/* 5952 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 5965
/* 5956 */    MCD_OPC_CheckPredicate, 8, 86, 30, // Skip to: 13726
/* 5960 */    MCD_OPC_Decode, 174, 8, 139, 1, // Opcode: MAX_S_W
/* 5965 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 5978
/* 5969 */    MCD_OPC_CheckPredicate, 8, 73, 30, // Skip to: 13726
/* 5973 */    MCD_OPC_Decode, 172, 8, 140, 1, // Opcode: MAX_S_D
/* 5978 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 5991
/* 5982 */    MCD_OPC_CheckPredicate, 8, 60, 30, // Skip to: 13726
/* 5986 */    MCD_OPC_Decode, 175, 8, 137, 1, // Opcode: MAX_U_B
/* 5991 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 6004
/* 5995 */    MCD_OPC_CheckPredicate, 8, 47, 30, // Skip to: 13726
/* 5999 */    MCD_OPC_Decode, 177, 8, 138, 1, // Opcode: MAX_U_H
/* 6004 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 6017
/* 6008 */    MCD_OPC_CheckPredicate, 8, 34, 30, // Skip to: 13726
/* 6012 */    MCD_OPC_Decode, 178, 8, 139, 1, // Opcode: MAX_U_W
/* 6017 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 6030
/* 6021 */    MCD_OPC_CheckPredicate, 8, 21, 30, // Skip to: 13726
/* 6025 */    MCD_OPC_Decode, 176, 8, 140, 1, // Opcode: MAX_U_D
/* 6030 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 6043
/* 6034 */    MCD_OPC_CheckPredicate, 8, 8, 30, // Skip to: 13726
/* 6038 */    MCD_OPC_Decode, 212, 8, 137, 1, // Opcode: MIN_S_B
/* 6043 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 6056
/* 6047 */    MCD_OPC_CheckPredicate, 8, 251, 29, // Skip to: 13726
/* 6051 */    MCD_OPC_Decode, 214, 8, 138, 1, // Opcode: MIN_S_H
/* 6056 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 6069
/* 6060 */    MCD_OPC_CheckPredicate, 8, 238, 29, // Skip to: 13726
/* 6064 */    MCD_OPC_Decode, 215, 8, 139, 1, // Opcode: MIN_S_W
/* 6069 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 6082
/* 6073 */    MCD_OPC_CheckPredicate, 8, 225, 29, // Skip to: 13726
/* 6077 */    MCD_OPC_Decode, 213, 8, 140, 1, // Opcode: MIN_S_D
/* 6082 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 6095
/* 6086 */    MCD_OPC_CheckPredicate, 8, 212, 29, // Skip to: 13726
/* 6090 */    MCD_OPC_Decode, 216, 8, 137, 1, // Opcode: MIN_U_B
/* 6095 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 6108
/* 6099 */    MCD_OPC_CheckPredicate, 8, 199, 29, // Skip to: 13726
/* 6103 */    MCD_OPC_Decode, 218, 8, 138, 1, // Opcode: MIN_U_H
/* 6108 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 6121
/* 6112 */    MCD_OPC_CheckPredicate, 8, 186, 29, // Skip to: 13726
/* 6116 */    MCD_OPC_Decode, 219, 8, 139, 1, // Opcode: MIN_U_W
/* 6121 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 6134
/* 6125 */    MCD_OPC_CheckPredicate, 8, 173, 29, // Skip to: 13726
/* 6129 */    MCD_OPC_Decode, 217, 8, 140, 1, // Opcode: MIN_U_D
/* 6134 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 6147
/* 6138 */    MCD_OPC_CheckPredicate, 8, 160, 29, // Skip to: 13726
/* 6142 */    MCD_OPC_Decode, 165, 8, 137, 1, // Opcode: MAX_A_B
/* 6147 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 6160
/* 6151 */    MCD_OPC_CheckPredicate, 8, 147, 29, // Skip to: 13726
/* 6155 */    MCD_OPC_Decode, 167, 8, 138, 1, // Opcode: MAX_A_H
/* 6160 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 6173
/* 6164 */    MCD_OPC_CheckPredicate, 8, 134, 29, // Skip to: 13726
/* 6168 */    MCD_OPC_Decode, 168, 8, 139, 1, // Opcode: MAX_A_W
/* 6173 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 6186
/* 6177 */    MCD_OPC_CheckPredicate, 8, 121, 29, // Skip to: 13726
/* 6181 */    MCD_OPC_Decode, 166, 8, 140, 1, // Opcode: MAX_A_D
/* 6186 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 6199
/* 6190 */    MCD_OPC_CheckPredicate, 8, 108, 29, // Skip to: 13726
/* 6194 */    MCD_OPC_Decode, 206, 8, 137, 1, // Opcode: MIN_A_B
/* 6199 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 6212
/* 6203 */    MCD_OPC_CheckPredicate, 8, 95, 29, // Skip to: 13726
/* 6207 */    MCD_OPC_Decode, 208, 8, 138, 1, // Opcode: MIN_A_H
/* 6212 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 6225
/* 6216 */    MCD_OPC_CheckPredicate, 8, 82, 29, // Skip to: 13726
/* 6220 */    MCD_OPC_Decode, 209, 8, 139, 1, // Opcode: MIN_A_W
/* 6225 */    MCD_OPC_FilterValue, 31, 73, 29, // Skip to: 13726
/* 6229 */    MCD_OPC_CheckPredicate, 8, 69, 29, // Skip to: 13726
/* 6233 */    MCD_OPC_Decode, 207, 8, 140, 1, // Opcode: MIN_A_D
/* 6238 */    MCD_OPC_FilterValue, 15, 7, 1, // Skip to: 6505
/* 6242 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 6245 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 6258
/* 6249 */    MCD_OPC_CheckPredicate, 8, 49, 29, // Skip to: 13726
/* 6253 */    MCD_OPC_Decode, 234, 2, 137, 1, // Opcode: CEQ_B
/* 6258 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 6271
/* 6262 */    MCD_OPC_CheckPredicate, 8, 36, 29, // Skip to: 13726
/* 6266 */    MCD_OPC_Decode, 236, 2, 138, 1, // Opcode: CEQ_H
/* 6271 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 6284
/* 6275 */    MCD_OPC_CheckPredicate, 8, 23, 29, // Skip to: 13726
/* 6279 */    MCD_OPC_Decode, 237, 2, 139, 1, // Opcode: CEQ_W
/* 6284 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 6297
/* 6288 */    MCD_OPC_CheckPredicate, 8, 10, 29, // Skip to: 13726
/* 6292 */    MCD_OPC_Decode, 235, 2, 140, 1, // Opcode: CEQ_D
/* 6297 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 6310
/* 6301 */    MCD_OPC_CheckPredicate, 8, 253, 28, // Skip to: 13726
/* 6305 */    MCD_OPC_Decode, 144, 3, 137, 1, // Opcode: CLT_S_B
/* 6310 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 6323
/* 6314 */    MCD_OPC_CheckPredicate, 8, 240, 28, // Skip to: 13726
/* 6318 */    MCD_OPC_Decode, 146, 3, 138, 1, // Opcode: CLT_S_H
/* 6323 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 6336
/* 6327 */    MCD_OPC_CheckPredicate, 8, 227, 28, // Skip to: 13726
/* 6331 */    MCD_OPC_Decode, 147, 3, 139, 1, // Opcode: CLT_S_W
/* 6336 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 6349
/* 6340 */    MCD_OPC_CheckPredicate, 8, 214, 28, // Skip to: 13726
/* 6344 */    MCD_OPC_Decode, 145, 3, 140, 1, // Opcode: CLT_S_D
/* 6349 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 6362
/* 6353 */    MCD_OPC_CheckPredicate, 8, 201, 28, // Skip to: 13726
/* 6357 */    MCD_OPC_Decode, 148, 3, 137, 1, // Opcode: CLT_U_B
/* 6362 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 6375
/* 6366 */    MCD_OPC_CheckPredicate, 8, 188, 28, // Skip to: 13726
/* 6370 */    MCD_OPC_Decode, 150, 3, 138, 1, // Opcode: CLT_U_H
/* 6375 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 6388
/* 6379 */    MCD_OPC_CheckPredicate, 8, 175, 28, // Skip to: 13726
/* 6383 */    MCD_OPC_Decode, 151, 3, 139, 1, // Opcode: CLT_U_W
/* 6388 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 6401
/* 6392 */    MCD_OPC_CheckPredicate, 8, 162, 28, // Skip to: 13726
/* 6396 */    MCD_OPC_Decode, 149, 3, 140, 1, // Opcode: CLT_U_D
/* 6401 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 6414
/* 6405 */    MCD_OPC_CheckPredicate, 8, 149, 28, // Skip to: 13726
/* 6409 */    MCD_OPC_Decode, 253, 2, 137, 1, // Opcode: CLE_S_B
/* 6414 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 6427
/* 6418 */    MCD_OPC_CheckPredicate, 8, 136, 28, // Skip to: 13726
/* 6422 */    MCD_OPC_Decode, 255, 2, 138, 1, // Opcode: CLE_S_H
/* 6427 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 6440
/* 6431 */    MCD_OPC_CheckPredicate, 8, 123, 28, // Skip to: 13726
/* 6435 */    MCD_OPC_Decode, 128, 3, 139, 1, // Opcode: CLE_S_W
/* 6440 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 6453
/* 6444 */    MCD_OPC_CheckPredicate, 8, 110, 28, // Skip to: 13726
/* 6448 */    MCD_OPC_Decode, 254, 2, 140, 1, // Opcode: CLE_S_D
/* 6453 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 6466
/* 6457 */    MCD_OPC_CheckPredicate, 8, 97, 28, // Skip to: 13726
/* 6461 */    MCD_OPC_Decode, 129, 3, 137, 1, // Opcode: CLE_U_B
/* 6466 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 6479
/* 6470 */    MCD_OPC_CheckPredicate, 8, 84, 28, // Skip to: 13726
/* 6474 */    MCD_OPC_Decode, 131, 3, 138, 1, // Opcode: CLE_U_H
/* 6479 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 6492
/* 6483 */    MCD_OPC_CheckPredicate, 8, 71, 28, // Skip to: 13726
/* 6487 */    MCD_OPC_Decode, 132, 3, 139, 1, // Opcode: CLE_U_W
/* 6492 */    MCD_OPC_FilterValue, 23, 62, 28, // Skip to: 13726
/* 6496 */    MCD_OPC_CheckPredicate, 8, 58, 28, // Skip to: 13726
/* 6500 */    MCD_OPC_Decode, 130, 3, 140, 1, // Opcode: CLE_U_D
/* 6505 */    MCD_OPC_FilterValue, 16, 147, 1, // Skip to: 6912
/* 6509 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 6512 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 6524
/* 6516 */    MCD_OPC_CheckPredicate, 8, 38, 28, // Skip to: 13726
/* 6520 */    MCD_OPC_Decode, 68, 137, 1, // Opcode: ADD_A_B
/* 6524 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 6536
/* 6528 */    MCD_OPC_CheckPredicate, 8, 26, 28, // Skip to: 13726
/* 6532 */    MCD_OPC_Decode, 70, 138, 1, // Opcode: ADD_A_H
/* 6536 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 6548
/* 6540 */    MCD_OPC_CheckPredicate, 8, 14, 28, // Skip to: 13726
/* 6544 */    MCD_OPC_Decode, 71, 139, 1, // Opcode: ADD_A_W
/* 6548 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 6560
/* 6552 */    MCD_OPC_CheckPredicate, 8, 2, 28, // Skip to: 13726
/* 6556 */    MCD_OPC_Decode, 69, 140, 1, // Opcode: ADD_A_D
/* 6560 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 6572
/* 6564 */    MCD_OPC_CheckPredicate, 8, 246, 27, // Skip to: 13726
/* 6568 */    MCD_OPC_Decode, 40, 137, 1, // Opcode: ADDS_A_B
/* 6572 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 6584
/* 6576 */    MCD_OPC_CheckPredicate, 8, 234, 27, // Skip to: 13726
/* 6580 */    MCD_OPC_Decode, 42, 138, 1, // Opcode: ADDS_A_H
/* 6584 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 6596
/* 6588 */    MCD_OPC_CheckPredicate, 8, 222, 27, // Skip to: 13726
/* 6592 */    MCD_OPC_Decode, 43, 139, 1, // Opcode: ADDS_A_W
/* 6596 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 6608
/* 6600 */    MCD_OPC_CheckPredicate, 8, 210, 27, // Skip to: 13726
/* 6604 */    MCD_OPC_Decode, 41, 140, 1, // Opcode: ADDS_A_D
/* 6608 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 6620
/* 6612 */    MCD_OPC_CheckPredicate, 8, 198, 27, // Skip to: 13726
/* 6616 */    MCD_OPC_Decode, 44, 137, 1, // Opcode: ADDS_S_B
/* 6620 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 6632
/* 6624 */    MCD_OPC_CheckPredicate, 8, 186, 27, // Skip to: 13726
/* 6628 */    MCD_OPC_Decode, 46, 138, 1, // Opcode: ADDS_S_H
/* 6632 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 6644
/* 6636 */    MCD_OPC_CheckPredicate, 8, 174, 27, // Skip to: 13726
/* 6640 */    MCD_OPC_Decode, 47, 139, 1, // Opcode: ADDS_S_W
/* 6644 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 6656
/* 6648 */    MCD_OPC_CheckPredicate, 8, 162, 27, // Skip to: 13726
/* 6652 */    MCD_OPC_Decode, 45, 140, 1, // Opcode: ADDS_S_D
/* 6656 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 6668
/* 6660 */    MCD_OPC_CheckPredicate, 8, 150, 27, // Skip to: 13726
/* 6664 */    MCD_OPC_Decode, 48, 137, 1, // Opcode: ADDS_U_B
/* 6668 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 6680
/* 6672 */    MCD_OPC_CheckPredicate, 8, 138, 27, // Skip to: 13726
/* 6676 */    MCD_OPC_Decode, 50, 138, 1, // Opcode: ADDS_U_H
/* 6680 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 6692
/* 6684 */    MCD_OPC_CheckPredicate, 8, 126, 27, // Skip to: 13726
/* 6688 */    MCD_OPC_Decode, 51, 139, 1, // Opcode: ADDS_U_W
/* 6692 */    MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 6704
/* 6696 */    MCD_OPC_CheckPredicate, 8, 114, 27, // Skip to: 13726
/* 6700 */    MCD_OPC_Decode, 49, 140, 1, // Opcode: ADDS_U_D
/* 6704 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 6717
/* 6708 */    MCD_OPC_CheckPredicate, 8, 102, 27, // Skip to: 13726
/* 6712 */    MCD_OPC_Decode, 147, 1, 137, 1, // Opcode: AVE_S_B
/* 6717 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 6730
/* 6721 */    MCD_OPC_CheckPredicate, 8, 89, 27, // Skip to: 13726
/* 6725 */    MCD_OPC_Decode, 149, 1, 138, 1, // Opcode: AVE_S_H
/* 6730 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 6743
/* 6734 */    MCD_OPC_CheckPredicate, 8, 76, 27, // Skip to: 13726
/* 6738 */    MCD_OPC_Decode, 150, 1, 139, 1, // Opcode: AVE_S_W
/* 6743 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 6756
/* 6747 */    MCD_OPC_CheckPredicate, 8, 63, 27, // Skip to: 13726
/* 6751 */    MCD_OPC_Decode, 148, 1, 140, 1, // Opcode: AVE_S_D
/* 6756 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 6769
/* 6760 */    MCD_OPC_CheckPredicate, 8, 50, 27, // Skip to: 13726
/* 6764 */    MCD_OPC_Decode, 151, 1, 137, 1, // Opcode: AVE_U_B
/* 6769 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 6782
/* 6773 */    MCD_OPC_CheckPredicate, 8, 37, 27, // Skip to: 13726
/* 6777 */    MCD_OPC_Decode, 153, 1, 138, 1, // Opcode: AVE_U_H
/* 6782 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 6795
/* 6786 */    MCD_OPC_CheckPredicate, 8, 24, 27, // Skip to: 13726
/* 6790 */    MCD_OPC_Decode, 154, 1, 139, 1, // Opcode: AVE_U_W
/* 6795 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 6808
/* 6799 */    MCD_OPC_CheckPredicate, 8, 11, 27, // Skip to: 13726
/* 6803 */    MCD_OPC_Decode, 152, 1, 140, 1, // Opcode: AVE_U_D
/* 6808 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 6821
/* 6812 */    MCD_OPC_CheckPredicate, 8, 254, 26, // Skip to: 13726
/* 6816 */    MCD_OPC_Decode, 139, 1, 137, 1, // Opcode: AVER_S_B
/* 6821 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 6834
/* 6825 */    MCD_OPC_CheckPredicate, 8, 241, 26, // Skip to: 13726
/* 6829 */    MCD_OPC_Decode, 141, 1, 138, 1, // Opcode: AVER_S_H
/* 6834 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 6847
/* 6838 */    MCD_OPC_CheckPredicate, 8, 228, 26, // Skip to: 13726
/* 6842 */    MCD_OPC_Decode, 142, 1, 139, 1, // Opcode: AVER_S_W
/* 6847 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 6860
/* 6851 */    MCD_OPC_CheckPredicate, 8, 215, 26, // Skip to: 13726
/* 6855 */    MCD_OPC_Decode, 140, 1, 140, 1, // Opcode: AVER_S_D
/* 6860 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 6873
/* 6864 */    MCD_OPC_CheckPredicate, 8, 202, 26, // Skip to: 13726
/* 6868 */    MCD_OPC_Decode, 143, 1, 137, 1, // Opcode: AVER_U_B
/* 6873 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 6886
/* 6877 */    MCD_OPC_CheckPredicate, 8, 189, 26, // Skip to: 13726
/* 6881 */    MCD_OPC_Decode, 145, 1, 138, 1, // Opcode: AVER_U_H
/* 6886 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 6899
/* 6890 */    MCD_OPC_CheckPredicate, 8, 176, 26, // Skip to: 13726
/* 6894 */    MCD_OPC_Decode, 146, 1, 139, 1, // Opcode: AVER_U_W
/* 6899 */    MCD_OPC_FilterValue, 31, 167, 26, // Skip to: 13726
/* 6903 */    MCD_OPC_CheckPredicate, 8, 163, 26, // Skip to: 13726
/* 6907 */    MCD_OPC_Decode, 144, 1, 140, 1, // Opcode: AVER_U_D
/* 6912 */    MCD_OPC_FilterValue, 17, 51, 1, // Skip to: 7223
/* 6916 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 6919 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 6932
/* 6923 */    MCD_OPC_CheckPredicate, 8, 143, 26, // Skip to: 13726
/* 6927 */    MCD_OPC_Decode, 206, 12, 137, 1, // Opcode: SUBS_S_B
/* 6932 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 6945
/* 6936 */    MCD_OPC_CheckPredicate, 8, 130, 26, // Skip to: 13726
/* 6940 */    MCD_OPC_Decode, 208, 12, 138, 1, // Opcode: SUBS_S_H
/* 6945 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 6958
/* 6949 */    MCD_OPC_CheckPredicate, 8, 117, 26, // Skip to: 13726
/* 6953 */    MCD_OPC_Decode, 209, 12, 139, 1, // Opcode: SUBS_S_W
/* 6958 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 6971
/* 6962 */    MCD_OPC_CheckPredicate, 8, 104, 26, // Skip to: 13726
/* 6966 */    MCD_OPC_Decode, 207, 12, 140, 1, // Opcode: SUBS_S_D
/* 6971 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 6984
/* 6975 */    MCD_OPC_CheckPredicate, 8, 91, 26, // Skip to: 13726
/* 6979 */    MCD_OPC_Decode, 210, 12, 137, 1, // Opcode: SUBS_U_B
/* 6984 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 6997
/* 6988 */    MCD_OPC_CheckPredicate, 8, 78, 26, // Skip to: 13726
/* 6992 */    MCD_OPC_Decode, 212, 12, 138, 1, // Opcode: SUBS_U_H
/* 6997 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 7010
/* 7001 */    MCD_OPC_CheckPredicate, 8, 65, 26, // Skip to: 13726
/* 7005 */    MCD_OPC_Decode, 213, 12, 139, 1, // Opcode: SUBS_U_W
/* 7010 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 7023
/* 7014 */    MCD_OPC_CheckPredicate, 8, 52, 26, // Skip to: 13726
/* 7018 */    MCD_OPC_Decode, 211, 12, 140, 1, // Opcode: SUBS_U_D
/* 7023 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 7036
/* 7027 */    MCD_OPC_CheckPredicate, 8, 39, 26, // Skip to: 13726
/* 7031 */    MCD_OPC_Decode, 198, 12, 137, 1, // Opcode: SUBSUS_U_B
/* 7036 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 7049
/* 7040 */    MCD_OPC_CheckPredicate, 8, 26, 26, // Skip to: 13726
/* 7044 */    MCD_OPC_Decode, 200, 12, 138, 1, // Opcode: SUBSUS_U_H
/* 7049 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 7062
/* 7053 */    MCD_OPC_CheckPredicate, 8, 13, 26, // Skip to: 13726
/* 7057 */    MCD_OPC_Decode, 201, 12, 139, 1, // Opcode: SUBSUS_U_W
/* 7062 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 7075
/* 7066 */    MCD_OPC_CheckPredicate, 8, 0, 26, // Skip to: 13726
/* 7070 */    MCD_OPC_Decode, 199, 12, 140, 1, // Opcode: SUBSUS_U_D
/* 7075 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 7088
/* 7079 */    MCD_OPC_CheckPredicate, 8, 243, 25, // Skip to: 13726
/* 7083 */    MCD_OPC_Decode, 202, 12, 137, 1, // Opcode: SUBSUU_S_B
/* 7088 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 7101
/* 7092 */    MCD_OPC_CheckPredicate, 8, 230, 25, // Skip to: 13726
/* 7096 */    MCD_OPC_Decode, 204, 12, 138, 1, // Opcode: SUBSUU_S_H
/* 7101 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 7114
/* 7105 */    MCD_OPC_CheckPredicate, 8, 217, 25, // Skip to: 13726
/* 7109 */    MCD_OPC_Decode, 205, 12, 139, 1, // Opcode: SUBSUU_S_W
/* 7114 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 7127
/* 7118 */    MCD_OPC_CheckPredicate, 8, 204, 25, // Skip to: 13726
/* 7122 */    MCD_OPC_Decode, 203, 12, 140, 1, // Opcode: SUBSUU_S_D
/* 7127 */    MCD_OPC_FilterValue, 16, 8, 0, // Skip to: 7139
/* 7131 */    MCD_OPC_CheckPredicate, 8, 191, 25, // Skip to: 13726
/* 7135 */    MCD_OPC_Decode, 97, 137, 1, // Opcode: ASUB_S_B
/* 7139 */    MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 7151
/* 7143 */    MCD_OPC_CheckPredicate, 8, 179, 25, // Skip to: 13726
/* 7147 */    MCD_OPC_Decode, 99, 138, 1, // Opcode: ASUB_S_H
/* 7151 */    MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 7163
/* 7155 */    MCD_OPC_CheckPredicate, 8, 167, 25, // Skip to: 13726
/* 7159 */    MCD_OPC_Decode, 100, 139, 1, // Opcode: ASUB_S_W
/* 7163 */    MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 7175
/* 7167 */    MCD_OPC_CheckPredicate, 8, 155, 25, // Skip to: 13726
/* 7171 */    MCD_OPC_Decode, 98, 140, 1, // Opcode: ASUB_S_D
/* 7175 */    MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 7187
/* 7179 */    MCD_OPC_CheckPredicate, 8, 143, 25, // Skip to: 13726
/* 7183 */    MCD_OPC_Decode, 101, 137, 1, // Opcode: ASUB_U_B
/* 7187 */    MCD_OPC_FilterValue, 21, 8, 0, // Skip to: 7199
/* 7191 */    MCD_OPC_CheckPredicate, 8, 131, 25, // Skip to: 13726
/* 7195 */    MCD_OPC_Decode, 103, 138, 1, // Opcode: ASUB_U_H
/* 7199 */    MCD_OPC_FilterValue, 22, 8, 0, // Skip to: 7211
/* 7203 */    MCD_OPC_CheckPredicate, 8, 119, 25, // Skip to: 13726
/* 7207 */    MCD_OPC_Decode, 104, 139, 1, // Opcode: ASUB_U_W
/* 7211 */    MCD_OPC_FilterValue, 23, 111, 25, // Skip to: 13726
/* 7215 */    MCD_OPC_CheckPredicate, 8, 107, 25, // Skip to: 13726
/* 7219 */    MCD_OPC_Decode, 102, 140, 1, // Opcode: ASUB_U_D
/* 7223 */    MCD_OPC_FilterValue, 18, 111, 1, // Skip to: 7594
/* 7227 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 7230 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 7243
/* 7234 */    MCD_OPC_CheckPredicate, 8, 88, 25, // Skip to: 13726
/* 7238 */    MCD_OPC_Decode, 213, 9, 137, 1, // Opcode: MULV_B
/* 7243 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 7256
/* 7247 */    MCD_OPC_CheckPredicate, 8, 75, 25, // Skip to: 13726
/* 7251 */    MCD_OPC_Decode, 215, 9, 138, 1, // Opcode: MULV_H
/* 7256 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 7269
/* 7260 */    MCD_OPC_CheckPredicate, 8, 62, 25, // Skip to: 13726
/* 7264 */    MCD_OPC_Decode, 216, 9, 139, 1, // Opcode: MULV_W
/* 7269 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 7282
/* 7273 */    MCD_OPC_CheckPredicate, 8, 49, 25, // Skip to: 13726
/* 7277 */    MCD_OPC_Decode, 214, 9, 140, 1, // Opcode: MULV_D
/* 7282 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 7295
/* 7286 */    MCD_OPC_CheckPredicate, 8, 36, 25, // Skip to: 13726
/* 7290 */    MCD_OPC_Decode, 138, 8, 141, 1, // Opcode: MADDV_B
/* 7295 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 7308
/* 7299 */    MCD_OPC_CheckPredicate, 8, 23, 25, // Skip to: 13726
/* 7303 */    MCD_OPC_Decode, 140, 8, 142, 1, // Opcode: MADDV_H
/* 7308 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 7321
/* 7312 */    MCD_OPC_CheckPredicate, 8, 10, 25, // Skip to: 13726
/* 7316 */    MCD_OPC_Decode, 141, 8, 143, 1, // Opcode: MADDV_W
/* 7321 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 7334
/* 7325 */    MCD_OPC_CheckPredicate, 8, 253, 24, // Skip to: 13726
/* 7329 */    MCD_OPC_Decode, 139, 8, 144, 1, // Opcode: MADDV_D
/* 7334 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 7347
/* 7338 */    MCD_OPC_CheckPredicate, 8, 240, 24, // Skip to: 13726
/* 7342 */    MCD_OPC_Decode, 156, 9, 141, 1, // Opcode: MSUBV_B
/* 7347 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 7360
/* 7351 */    MCD_OPC_CheckPredicate, 8, 227, 24, // Skip to: 13726
/* 7355 */    MCD_OPC_Decode, 158, 9, 142, 1, // Opcode: MSUBV_H
/* 7360 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 7373
/* 7364 */    MCD_OPC_CheckPredicate, 8, 214, 24, // Skip to: 13726
/* 7368 */    MCD_OPC_Decode, 159, 9, 143, 1, // Opcode: MSUBV_W
/* 7373 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 7386
/* 7377 */    MCD_OPC_CheckPredicate, 8, 201, 24, // Skip to: 13726
/* 7381 */    MCD_OPC_Decode, 157, 9, 144, 1, // Opcode: MSUBV_D
/* 7386 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 7399
/* 7390 */    MCD_OPC_CheckPredicate, 8, 188, 24, // Skip to: 13726
/* 7394 */    MCD_OPC_Decode, 185, 4, 137, 1, // Opcode: DIV_S_B
/* 7399 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 7412
/* 7403 */    MCD_OPC_CheckPredicate, 8, 175, 24, // Skip to: 13726
/* 7407 */    MCD_OPC_Decode, 187, 4, 138, 1, // Opcode: DIV_S_H
/* 7412 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 7425
/* 7416 */    MCD_OPC_CheckPredicate, 8, 162, 24, // Skip to: 13726
/* 7420 */    MCD_OPC_Decode, 188, 4, 139, 1, // Opcode: DIV_S_W
/* 7425 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 7438
/* 7429 */    MCD_OPC_CheckPredicate, 8, 149, 24, // Skip to: 13726
/* 7433 */    MCD_OPC_Decode, 186, 4, 140, 1, // Opcode: DIV_S_D
/* 7438 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 7451
/* 7442 */    MCD_OPC_CheckPredicate, 8, 136, 24, // Skip to: 13726
/* 7446 */    MCD_OPC_Decode, 189, 4, 137, 1, // Opcode: DIV_U_B
/* 7451 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 7464
/* 7455 */    MCD_OPC_CheckPredicate, 8, 123, 24, // Skip to: 13726
/* 7459 */    MCD_OPC_Decode, 191, 4, 138, 1, // Opcode: DIV_U_H
/* 7464 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 7477
/* 7468 */    MCD_OPC_CheckPredicate, 8, 110, 24, // Skip to: 13726
/* 7472 */    MCD_OPC_Decode, 192, 4, 139, 1, // Opcode: DIV_U_W
/* 7477 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 7490
/* 7481 */    MCD_OPC_CheckPredicate, 8, 97, 24, // Skip to: 13726
/* 7485 */    MCD_OPC_Decode, 190, 4, 140, 1, // Opcode: DIV_U_D
/* 7490 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 7503
/* 7494 */    MCD_OPC_CheckPredicate, 8, 84, 24, // Skip to: 13726
/* 7498 */    MCD_OPC_Decode, 225, 8, 137, 1, // Opcode: MOD_S_B
/* 7503 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 7516
/* 7507 */    MCD_OPC_CheckPredicate, 8, 71, 24, // Skip to: 13726
/* 7511 */    MCD_OPC_Decode, 227, 8, 138, 1, // Opcode: MOD_S_H
/* 7516 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 7529
/* 7520 */    MCD_OPC_CheckPredicate, 8, 58, 24, // Skip to: 13726
/* 7524 */    MCD_OPC_Decode, 228, 8, 139, 1, // Opcode: MOD_S_W
/* 7529 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 7542
/* 7533 */    MCD_OPC_CheckPredicate, 8, 45, 24, // Skip to: 13726
/* 7537 */    MCD_OPC_Decode, 226, 8, 140, 1, // Opcode: MOD_S_D
/* 7542 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 7555
/* 7546 */    MCD_OPC_CheckPredicate, 8, 32, 24, // Skip to: 13726
/* 7550 */    MCD_OPC_Decode, 229, 8, 137, 1, // Opcode: MOD_U_B
/* 7555 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 7568
/* 7559 */    MCD_OPC_CheckPredicate, 8, 19, 24, // Skip to: 13726
/* 7563 */    MCD_OPC_Decode, 231, 8, 138, 1, // Opcode: MOD_U_H
/* 7568 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 7581
/* 7572 */    MCD_OPC_CheckPredicate, 8, 6, 24, // Skip to: 13726
/* 7576 */    MCD_OPC_Decode, 232, 8, 139, 1, // Opcode: MOD_U_W
/* 7581 */    MCD_OPC_FilterValue, 31, 253, 23, // Skip to: 13726
/* 7585 */    MCD_OPC_CheckPredicate, 8, 249, 23, // Skip to: 13726
/* 7589 */    MCD_OPC_Decode, 230, 8, 140, 1, // Opcode: MOD_U_D
/* 7594 */    MCD_OPC_FilterValue, 19, 237, 0, // Skip to: 7835
/* 7598 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 7601 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 7614
/* 7605 */    MCD_OPC_CheckPredicate, 8, 229, 23, // Skip to: 13726
/* 7609 */    MCD_OPC_Decode, 212, 4, 145, 1, // Opcode: DOTP_S_H
/* 7614 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 7627
/* 7618 */    MCD_OPC_CheckPredicate, 8, 216, 23, // Skip to: 13726
/* 7622 */    MCD_OPC_Decode, 213, 4, 146, 1, // Opcode: DOTP_S_W
/* 7627 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 7640
/* 7631 */    MCD_OPC_CheckPredicate, 8, 203, 23, // Skip to: 13726
/* 7635 */    MCD_OPC_Decode, 211, 4, 147, 1, // Opcode: DOTP_S_D
/* 7640 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 7653
/* 7644 */    MCD_OPC_CheckPredicate, 8, 190, 23, // Skip to: 13726
/* 7648 */    MCD_OPC_Decode, 215, 4, 145, 1, // Opcode: DOTP_U_H
/* 7653 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 7666
/* 7657 */    MCD_OPC_CheckPredicate, 8, 177, 23, // Skip to: 13726
/* 7661 */    MCD_OPC_Decode, 216, 4, 146, 1, // Opcode: DOTP_U_W
/* 7666 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 7679
/* 7670 */    MCD_OPC_CheckPredicate, 8, 164, 23, // Skip to: 13726
/* 7674 */    MCD_OPC_Decode, 214, 4, 147, 1, // Opcode: DOTP_U_D
/* 7679 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 7692
/* 7683 */    MCD_OPC_CheckPredicate, 8, 151, 23, // Skip to: 13726
/* 7687 */    MCD_OPC_Decode, 218, 4, 148, 1, // Opcode: DPADD_S_H
/* 7692 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 7705
/* 7696 */    MCD_OPC_CheckPredicate, 8, 138, 23, // Skip to: 13726
/* 7700 */    MCD_OPC_Decode, 219, 4, 149, 1, // Opcode: DPADD_S_W
/* 7705 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 7718
/* 7709 */    MCD_OPC_CheckPredicate, 8, 125, 23, // Skip to: 13726
/* 7713 */    MCD_OPC_Decode, 217, 4, 150, 1, // Opcode: DPADD_S_D
/* 7718 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 7731
/* 7722 */    MCD_OPC_CheckPredicate, 8, 112, 23, // Skip to: 13726
/* 7726 */    MCD_OPC_Decode, 221, 4, 148, 1, // Opcode: DPADD_U_H
/* 7731 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 7744
/* 7735 */    MCD_OPC_CheckPredicate, 8, 99, 23, // Skip to: 13726
/* 7739 */    MCD_OPC_Decode, 222, 4, 149, 1, // Opcode: DPADD_U_W
/* 7744 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 7757
/* 7748 */    MCD_OPC_CheckPredicate, 8, 86, 23, // Skip to: 13726
/* 7752 */    MCD_OPC_Decode, 220, 4, 150, 1, // Opcode: DPADD_U_D
/* 7757 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 7770
/* 7761 */    MCD_OPC_CheckPredicate, 8, 73, 23, // Skip to: 13726
/* 7765 */    MCD_OPC_Decode, 237, 4, 148, 1, // Opcode: DPSUB_S_H
/* 7770 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 7783
/* 7774 */    MCD_OPC_CheckPredicate, 8, 60, 23, // Skip to: 13726
/* 7778 */    MCD_OPC_Decode, 238, 4, 149, 1, // Opcode: DPSUB_S_W
/* 7783 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 7796
/* 7787 */    MCD_OPC_CheckPredicate, 8, 47, 23, // Skip to: 13726
/* 7791 */    MCD_OPC_Decode, 236, 4, 150, 1, // Opcode: DPSUB_S_D
/* 7796 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 7809
/* 7800 */    MCD_OPC_CheckPredicate, 8, 34, 23, // Skip to: 13726
/* 7804 */    MCD_OPC_Decode, 240, 4, 148, 1, // Opcode: DPSUB_U_H
/* 7809 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 7822
/* 7813 */    MCD_OPC_CheckPredicate, 8, 21, 23, // Skip to: 13726
/* 7817 */    MCD_OPC_Decode, 241, 4, 149, 1, // Opcode: DPSUB_U_W
/* 7822 */    MCD_OPC_FilterValue, 23, 12, 23, // Skip to: 13726
/* 7826 */    MCD_OPC_CheckPredicate, 8, 8, 23, // Skip to: 13726
/* 7830 */    MCD_OPC_Decode, 239, 4, 150, 1, // Opcode: DPSUB_U_D
/* 7835 */    MCD_OPC_FilterValue, 20, 163, 1, // Skip to: 8258
/* 7839 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 7842 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 7855
/* 7846 */    MCD_OPC_CheckPredicate, 8, 244, 22, // Skip to: 13726
/* 7850 */    MCD_OPC_Decode, 221, 11, 151, 1, // Opcode: SLD_B
/* 7855 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 7868
/* 7859 */    MCD_OPC_CheckPredicate, 8, 231, 22, // Skip to: 13726
/* 7863 */    MCD_OPC_Decode, 223, 11, 152, 1, // Opcode: SLD_H
/* 7868 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 7881
/* 7872 */    MCD_OPC_CheckPredicate, 8, 218, 22, // Skip to: 13726
/* 7876 */    MCD_OPC_Decode, 224, 11, 153, 1, // Opcode: SLD_W
/* 7881 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 7894
/* 7885 */    MCD_OPC_CheckPredicate, 8, 205, 22, // Skip to: 13726
/* 7889 */    MCD_OPC_Decode, 222, 11, 154, 1, // Opcode: SLD_D
/* 7894 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 7907
/* 7898 */    MCD_OPC_CheckPredicate, 8, 192, 22, // Skip to: 13726
/* 7902 */    MCD_OPC_Decode, 135, 12, 155, 1, // Opcode: SPLAT_B
/* 7907 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 7920
/* 7911 */    MCD_OPC_CheckPredicate, 8, 179, 22, // Skip to: 13726
/* 7915 */    MCD_OPC_Decode, 137, 12, 156, 1, // Opcode: SPLAT_H
/* 7920 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 7933
/* 7924 */    MCD_OPC_CheckPredicate, 8, 166, 22, // Skip to: 13726
/* 7928 */    MCD_OPC_Decode, 138, 12, 157, 1, // Opcode: SPLAT_W
/* 7933 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 7946
/* 7937 */    MCD_OPC_CheckPredicate, 8, 153, 22, // Skip to: 13726
/* 7941 */    MCD_OPC_Decode, 136, 12, 158, 1, // Opcode: SPLAT_D
/* 7946 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 7959
/* 7950 */    MCD_OPC_CheckPredicate, 8, 140, 22, // Skip to: 13726
/* 7954 */    MCD_OPC_Decode, 149, 10, 137, 1, // Opcode: PCKEV_B
/* 7959 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 7972
/* 7963 */    MCD_OPC_CheckPredicate, 8, 127, 22, // Skip to: 13726
/* 7967 */    MCD_OPC_Decode, 151, 10, 138, 1, // Opcode: PCKEV_H
/* 7972 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 7985
/* 7976 */    MCD_OPC_CheckPredicate, 8, 114, 22, // Skip to: 13726
/* 7980 */    MCD_OPC_Decode, 152, 10, 139, 1, // Opcode: PCKEV_W
/* 7985 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 7998
/* 7989 */    MCD_OPC_CheckPredicate, 8, 101, 22, // Skip to: 13726
/* 7993 */    MCD_OPC_Decode, 150, 10, 140, 1, // Opcode: PCKEV_D
/* 7998 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 8011
/* 8002 */    MCD_OPC_CheckPredicate, 8, 88, 22, // Skip to: 13726
/* 8006 */    MCD_OPC_Decode, 153, 10, 137, 1, // Opcode: PCKOD_B
/* 8011 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 8024
/* 8015 */    MCD_OPC_CheckPredicate, 8, 75, 22, // Skip to: 13726
/* 8019 */    MCD_OPC_Decode, 155, 10, 138, 1, // Opcode: PCKOD_H
/* 8024 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 8037
/* 8028 */    MCD_OPC_CheckPredicate, 8, 62, 22, // Skip to: 13726
/* 8032 */    MCD_OPC_Decode, 156, 10, 139, 1, // Opcode: PCKOD_W
/* 8037 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 8050
/* 8041 */    MCD_OPC_CheckPredicate, 8, 49, 22, // Skip to: 13726
/* 8045 */    MCD_OPC_Decode, 154, 10, 140, 1, // Opcode: PCKOD_D
/* 8050 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 8063
/* 8054 */    MCD_OPC_CheckPredicate, 8, 36, 22, // Skip to: 13726
/* 8058 */    MCD_OPC_Decode, 216, 6, 137, 1, // Opcode: ILVL_B
/* 8063 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 8076
/* 8067 */    MCD_OPC_CheckPredicate, 8, 23, 22, // Skip to: 13726
/* 8071 */    MCD_OPC_Decode, 218, 6, 138, 1, // Opcode: ILVL_H
/* 8076 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 8089
/* 8080 */    MCD_OPC_CheckPredicate, 8, 10, 22, // Skip to: 13726
/* 8084 */    MCD_OPC_Decode, 219, 6, 139, 1, // Opcode: ILVL_W
/* 8089 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 8102
/* 8093 */    MCD_OPC_CheckPredicate, 8, 253, 21, // Skip to: 13726
/* 8097 */    MCD_OPC_Decode, 217, 6, 140, 1, // Opcode: ILVL_D
/* 8102 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 8115
/* 8106 */    MCD_OPC_CheckPredicate, 8, 240, 21, // Skip to: 13726
/* 8110 */    MCD_OPC_Decode, 224, 6, 137, 1, // Opcode: ILVR_B
/* 8115 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 8128
/* 8119 */    MCD_OPC_CheckPredicate, 8, 227, 21, // Skip to: 13726
/* 8123 */    MCD_OPC_Decode, 226, 6, 138, 1, // Opcode: ILVR_H
/* 8128 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 8141
/* 8132 */    MCD_OPC_CheckPredicate, 8, 214, 21, // Skip to: 13726
/* 8136 */    MCD_OPC_Decode, 227, 6, 139, 1, // Opcode: ILVR_W
/* 8141 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 8154
/* 8145 */    MCD_OPC_CheckPredicate, 8, 201, 21, // Skip to: 13726
/* 8149 */    MCD_OPC_Decode, 225, 6, 140, 1, // Opcode: ILVR_D
/* 8154 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 8167
/* 8158 */    MCD_OPC_CheckPredicate, 8, 188, 21, // Skip to: 13726
/* 8162 */    MCD_OPC_Decode, 212, 6, 137, 1, // Opcode: ILVEV_B
/* 8167 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 8180
/* 8171 */    MCD_OPC_CheckPredicate, 8, 175, 21, // Skip to: 13726
/* 8175 */    MCD_OPC_Decode, 214, 6, 138, 1, // Opcode: ILVEV_H
/* 8180 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 8193
/* 8184 */    MCD_OPC_CheckPredicate, 8, 162, 21, // Skip to: 13726
/* 8188 */    MCD_OPC_Decode, 215, 6, 139, 1, // Opcode: ILVEV_W
/* 8193 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 8206
/* 8197 */    MCD_OPC_CheckPredicate, 8, 149, 21, // Skip to: 13726
/* 8201 */    MCD_OPC_Decode, 213, 6, 140, 1, // Opcode: ILVEV_D
/* 8206 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 8219
/* 8210 */    MCD_OPC_CheckPredicate, 8, 136, 21, // Skip to: 13726
/* 8214 */    MCD_OPC_Decode, 220, 6, 137, 1, // Opcode: ILVOD_B
/* 8219 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 8232
/* 8223 */    MCD_OPC_CheckPredicate, 8, 123, 21, // Skip to: 13726
/* 8227 */    MCD_OPC_Decode, 222, 6, 138, 1, // Opcode: ILVOD_H
/* 8232 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 8245
/* 8236 */    MCD_OPC_CheckPredicate, 8, 110, 21, // Skip to: 13726
/* 8240 */    MCD_OPC_Decode, 223, 6, 139, 1, // Opcode: ILVOD_W
/* 8245 */    MCD_OPC_FilterValue, 31, 101, 21, // Skip to: 13726
/* 8249 */    MCD_OPC_CheckPredicate, 8, 97, 21, // Skip to: 13726
/* 8253 */    MCD_OPC_Decode, 221, 6, 140, 1, // Opcode: ILVOD_D
/* 8258 */    MCD_OPC_FilterValue, 21, 59, 1, // Skip to: 8577
/* 8262 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 8265 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8278
/* 8269 */    MCD_OPC_CheckPredicate, 8, 77, 21, // Skip to: 13726
/* 8273 */    MCD_OPC_Decode, 227, 13, 141, 1, // Opcode: VSHF_B
/* 8278 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 8291
/* 8282 */    MCD_OPC_CheckPredicate, 8, 64, 21, // Skip to: 13726
/* 8286 */    MCD_OPC_Decode, 229, 13, 142, 1, // Opcode: VSHF_H
/* 8291 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 8304
/* 8295 */    MCD_OPC_CheckPredicate, 8, 51, 21, // Skip to: 13726
/* 8299 */    MCD_OPC_Decode, 230, 13, 143, 1, // Opcode: VSHF_W
/* 8304 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 8317
/* 8308 */    MCD_OPC_CheckPredicate, 8, 38, 21, // Skip to: 13726
/* 8312 */    MCD_OPC_Decode, 228, 13, 144, 1, // Opcode: VSHF_D
/* 8317 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 8330
/* 8321 */    MCD_OPC_CheckPredicate, 8, 25, 21, // Skip to: 13726
/* 8325 */    MCD_OPC_Decode, 148, 12, 137, 1, // Opcode: SRAR_B
/* 8330 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 8343
/* 8334 */    MCD_OPC_CheckPredicate, 8, 12, 21, // Skip to: 13726
/* 8338 */    MCD_OPC_Decode, 150, 12, 138, 1, // Opcode: SRAR_H
/* 8343 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 8356
/* 8347 */    MCD_OPC_CheckPredicate, 8, 255, 20, // Skip to: 13726
/* 8351 */    MCD_OPC_Decode, 151, 12, 139, 1, // Opcode: SRAR_W
/* 8356 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 8369
/* 8360 */    MCD_OPC_CheckPredicate, 8, 242, 20, // Skip to: 13726
/* 8364 */    MCD_OPC_Decode, 149, 12, 140, 1, // Opcode: SRAR_D
/* 8369 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 8382
/* 8373 */    MCD_OPC_CheckPredicate, 8, 229, 20, // Skip to: 13726
/* 8377 */    MCD_OPC_Decode, 169, 12, 137, 1, // Opcode: SRLR_B
/* 8382 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 8395
/* 8386 */    MCD_OPC_CheckPredicate, 8, 216, 20, // Skip to: 13726
/* 8390 */    MCD_OPC_Decode, 171, 12, 138, 1, // Opcode: SRLR_H
/* 8395 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 8408
/* 8399 */    MCD_OPC_CheckPredicate, 8, 203, 20, // Skip to: 13726
/* 8403 */    MCD_OPC_Decode, 172, 12, 139, 1, // Opcode: SRLR_W
/* 8408 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 8421
/* 8412 */    MCD_OPC_CheckPredicate, 8, 190, 20, // Skip to: 13726
/* 8416 */    MCD_OPC_Decode, 170, 12, 140, 1, // Opcode: SRLR_D
/* 8421 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 8434
/* 8425 */    MCD_OPC_CheckPredicate, 8, 177, 20, // Skip to: 13726
/* 8429 */    MCD_OPC_Decode, 201, 6, 145, 1, // Opcode: HADD_S_H
/* 8434 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 8447
/* 8438 */    MCD_OPC_CheckPredicate, 8, 164, 20, // Skip to: 13726
/* 8442 */    MCD_OPC_Decode, 202, 6, 146, 1, // Opcode: HADD_S_W
/* 8447 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 8460
/* 8451 */    MCD_OPC_CheckPredicate, 8, 151, 20, // Skip to: 13726
/* 8455 */    MCD_OPC_Decode, 200, 6, 147, 1, // Opcode: HADD_S_D
/* 8460 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 8473
/* 8464 */    MCD_OPC_CheckPredicate, 8, 138, 20, // Skip to: 13726
/* 8468 */    MCD_OPC_Decode, 204, 6, 145, 1, // Opcode: HADD_U_H
/* 8473 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 8486
/* 8477 */    MCD_OPC_CheckPredicate, 8, 125, 20, // Skip to: 13726
/* 8481 */    MCD_OPC_Decode, 205, 6, 146, 1, // Opcode: HADD_U_W
/* 8486 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 8499
/* 8490 */    MCD_OPC_CheckPredicate, 8, 112, 20, // Skip to: 13726
/* 8494 */    MCD_OPC_Decode, 203, 6, 147, 1, // Opcode: HADD_U_D
/* 8499 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 8512
/* 8503 */    MCD_OPC_CheckPredicate, 8, 99, 20, // Skip to: 13726
/* 8507 */    MCD_OPC_Decode, 207, 6, 145, 1, // Opcode: HSUB_S_H
/* 8512 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 8525
/* 8516 */    MCD_OPC_CheckPredicate, 8, 86, 20, // Skip to: 13726
/* 8520 */    MCD_OPC_Decode, 208, 6, 146, 1, // Opcode: HSUB_S_W
/* 8525 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 8538
/* 8529 */    MCD_OPC_CheckPredicate, 8, 73, 20, // Skip to: 13726
/* 8533 */    MCD_OPC_Decode, 206, 6, 147, 1, // Opcode: HSUB_S_D
/* 8538 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 8551
/* 8542 */    MCD_OPC_CheckPredicate, 8, 60, 20, // Skip to: 13726
/* 8546 */    MCD_OPC_Decode, 210, 6, 145, 1, // Opcode: HSUB_U_H
/* 8551 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 8564
/* 8555 */    MCD_OPC_CheckPredicate, 8, 47, 20, // Skip to: 13726
/* 8559 */    MCD_OPC_Decode, 211, 6, 146, 1, // Opcode: HSUB_U_W
/* 8564 */    MCD_OPC_FilterValue, 31, 38, 20, // Skip to: 13726
/* 8568 */    MCD_OPC_CheckPredicate, 8, 34, 20, // Skip to: 13726
/* 8572 */    MCD_OPC_Decode, 209, 6, 147, 1, // Opcode: HSUB_U_D
/* 8577 */    MCD_OPC_FilterValue, 25, 230, 1, // Skip to: 9067
/* 8581 */    MCD_OPC_ExtractField, 20, 6,  // Inst{25-20} ...
/* 8584 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8597
/* 8588 */    MCD_OPC_CheckPredicate, 8, 14, 20, // Skip to: 13726
/* 8592 */    MCD_OPC_Decode, 217, 11, 159, 1, // Opcode: SLDI_B
/* 8597 */    MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 8616
/* 8601 */    MCD_OPC_CheckPredicate, 8, 1, 20, // Skip to: 13726
/* 8605 */    MCD_OPC_CheckField, 19, 1, 0, 251, 19, // Skip to: 13726
/* 8611 */    MCD_OPC_Decode, 219, 11, 160, 1, // Opcode: SLDI_H
/* 8616 */    MCD_OPC_FilterValue, 3, 54, 0, // Skip to: 8674
/* 8620 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 8623 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8636
/* 8627 */    MCD_OPC_CheckPredicate, 8, 231, 19, // Skip to: 13726
/* 8631 */    MCD_OPC_Decode, 220, 11, 161, 1, // Opcode: SLDI_W
/* 8636 */    MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 8655
/* 8640 */    MCD_OPC_CheckPredicate, 8, 218, 19, // Skip to: 13726
/* 8644 */    MCD_OPC_CheckField, 17, 1, 0, 212, 19, // Skip to: 13726
/* 8650 */    MCD_OPC_Decode, 218, 11, 162, 1, // Opcode: SLDI_D
/* 8655 */    MCD_OPC_FilterValue, 3, 203, 19, // Skip to: 13726
/* 8659 */    MCD_OPC_CheckPredicate, 8, 199, 19, // Skip to: 13726
/* 8663 */    MCD_OPC_CheckField, 16, 2, 2, 193, 19, // Skip to: 13726
/* 8669 */    MCD_OPC_Decode, 212, 3, 163, 1, // Opcode: CTCMSA
/* 8674 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 8687
/* 8678 */    MCD_OPC_CheckPredicate, 8, 180, 19, // Skip to: 13726
/* 8682 */    MCD_OPC_Decode, 131, 12, 164, 1, // Opcode: SPLATI_B
/* 8687 */    MCD_OPC_FilterValue, 6, 15, 0, // Skip to: 8706
/* 8691 */    MCD_OPC_CheckPredicate, 8, 167, 19, // Skip to: 13726
/* 8695 */    MCD_OPC_CheckField, 19, 1, 0, 161, 19, // Skip to: 13726
/* 8701 */    MCD_OPC_Decode, 133, 12, 165, 1, // Opcode: SPLATI_H
/* 8706 */    MCD_OPC_FilterValue, 7, 54, 0, // Skip to: 8764
/* 8710 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 8713 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8726
/* 8717 */    MCD_OPC_CheckPredicate, 8, 141, 19, // Skip to: 13726
/* 8721 */    MCD_OPC_Decode, 134, 12, 166, 1, // Opcode: SPLATI_W
/* 8726 */    MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 8745
/* 8730 */    MCD_OPC_CheckPredicate, 8, 128, 19, // Skip to: 13726
/* 8734 */    MCD_OPC_CheckField, 17, 1, 0, 122, 19, // Skip to: 13726
/* 8740 */    MCD_OPC_Decode, 132, 12, 167, 1, // Opcode: SPLATI_D
/* 8745 */    MCD_OPC_FilterValue, 3, 113, 19, // Skip to: 13726
/* 8749 */    MCD_OPC_CheckPredicate, 8, 109, 19, // Skip to: 13726
/* 8753 */    MCD_OPC_CheckField, 16, 2, 2, 103, 19, // Skip to: 13726
/* 8759 */    MCD_OPC_Decode, 240, 2, 168, 1, // Opcode: CFCMSA
/* 8764 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 8777
/* 8768 */    MCD_OPC_CheckPredicate, 8, 90, 19, // Skip to: 13726
/* 8772 */    MCD_OPC_Decode, 202, 3, 169, 1, // Opcode: COPY_S_B
/* 8777 */    MCD_OPC_FilterValue, 10, 15, 0, // Skip to: 8796
/* 8781 */    MCD_OPC_CheckPredicate, 8, 77, 19, // Skip to: 13726
/* 8785 */    MCD_OPC_CheckField, 19, 1, 0, 71, 19, // Skip to: 13726
/* 8791 */    MCD_OPC_Decode, 204, 3, 170, 1, // Opcode: COPY_S_H
/* 8796 */    MCD_OPC_FilterValue, 11, 54, 0, // Skip to: 8854
/* 8800 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 8803 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8816
/* 8807 */    MCD_OPC_CheckPredicate, 8, 51, 19, // Skip to: 13726
/* 8811 */    MCD_OPC_Decode, 205, 3, 171, 1, // Opcode: COPY_S_W
/* 8816 */    MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 8835
/* 8820 */    MCD_OPC_CheckPredicate, 14, 38, 19, // Skip to: 13726
/* 8824 */    MCD_OPC_CheckField, 17, 1, 0, 32, 19, // Skip to: 13726
/* 8830 */    MCD_OPC_Decode, 203, 3, 172, 1, // Opcode: COPY_S_D
/* 8835 */    MCD_OPC_FilterValue, 3, 23, 19, // Skip to: 13726
/* 8839 */    MCD_OPC_CheckPredicate, 8, 19, 19, // Skip to: 13726
/* 8843 */    MCD_OPC_CheckField, 16, 2, 2, 13, 19, // Skip to: 13726
/* 8849 */    MCD_OPC_Decode, 235, 8, 173, 1, // Opcode: MOVE_V
/* 8854 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 8867
/* 8858 */    MCD_OPC_CheckPredicate, 8, 0, 19, // Skip to: 13726
/* 8862 */    MCD_OPC_Decode, 206, 3, 169, 1, // Opcode: COPY_U_B
/* 8867 */    MCD_OPC_FilterValue, 14, 15, 0, // Skip to: 8886
/* 8871 */    MCD_OPC_CheckPredicate, 8, 243, 18, // Skip to: 13726
/* 8875 */    MCD_OPC_CheckField, 19, 1, 0, 237, 18, // Skip to: 13726
/* 8881 */    MCD_OPC_Decode, 208, 3, 170, 1, // Opcode: COPY_U_H
/* 8886 */    MCD_OPC_FilterValue, 15, 35, 0, // Skip to: 8925
/* 8890 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 8893 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8906
/* 8897 */    MCD_OPC_CheckPredicate, 8, 217, 18, // Skip to: 13726
/* 8901 */    MCD_OPC_Decode, 209, 3, 171, 1, // Opcode: COPY_U_W
/* 8906 */    MCD_OPC_FilterValue, 2, 208, 18, // Skip to: 13726
/* 8910 */    MCD_OPC_CheckPredicate, 14, 204, 18, // Skip to: 13726
/* 8914 */    MCD_OPC_CheckField, 17, 1, 0, 198, 18, // Skip to: 13726
/* 8920 */    MCD_OPC_Decode, 207, 3, 172, 1, // Opcode: COPY_U_D
/* 8925 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 8938
/* 8929 */    MCD_OPC_CheckPredicate, 8, 185, 18, // Skip to: 13726
/* 8933 */    MCD_OPC_Decode, 229, 6, 174, 1, // Opcode: INSERT_B
/* 8938 */    MCD_OPC_FilterValue, 18, 15, 0, // Skip to: 8957
/* 8942 */    MCD_OPC_CheckPredicate, 8, 172, 18, // Skip to: 13726
/* 8946 */    MCD_OPC_CheckField, 19, 1, 0, 166, 18, // Skip to: 13726
/* 8952 */    MCD_OPC_Decode, 237, 6, 175, 1, // Opcode: INSERT_H
/* 8957 */    MCD_OPC_FilterValue, 19, 35, 0, // Skip to: 8996
/* 8961 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 8964 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 8977
/* 8968 */    MCD_OPC_CheckPredicate, 8, 146, 18, // Skip to: 13726
/* 8972 */    MCD_OPC_Decode, 239, 6, 176, 1, // Opcode: INSERT_W
/* 8977 */    MCD_OPC_FilterValue, 2, 137, 18, // Skip to: 13726
/* 8981 */    MCD_OPC_CheckPredicate, 14, 133, 18, // Skip to: 13726
/* 8985 */    MCD_OPC_CheckField, 17, 1, 0, 127, 18, // Skip to: 13726
/* 8991 */    MCD_OPC_Decode, 231, 6, 177, 1, // Opcode: INSERT_D
/* 8996 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 9009
/* 9000 */    MCD_OPC_CheckPredicate, 8, 114, 18, // Skip to: 13726
/* 9004 */    MCD_OPC_Decode, 242, 6, 178, 1, // Opcode: INSVE_B
/* 9009 */    MCD_OPC_FilterValue, 22, 15, 0, // Skip to: 9028
/* 9013 */    MCD_OPC_CheckPredicate, 8, 101, 18, // Skip to: 13726
/* 9017 */    MCD_OPC_CheckField, 19, 1, 0, 95, 18, // Skip to: 13726
/* 9023 */    MCD_OPC_Decode, 244, 6, 178, 1, // Opcode: INSVE_H
/* 9028 */    MCD_OPC_FilterValue, 23, 86, 18, // Skip to: 13726
/* 9032 */    MCD_OPC_ExtractField, 18, 2,  // Inst{19-18} ...
/* 9035 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 9048
/* 9039 */    MCD_OPC_CheckPredicate, 8, 75, 18, // Skip to: 13726
/* 9043 */    MCD_OPC_Decode, 245, 6, 178, 1, // Opcode: INSVE_W
/* 9048 */    MCD_OPC_FilterValue, 2, 66, 18, // Skip to: 13726
/* 9052 */    MCD_OPC_CheckPredicate, 8, 62, 18, // Skip to: 13726
/* 9056 */    MCD_OPC_CheckField, 17, 1, 0, 56, 18, // Skip to: 13726
/* 9062 */    MCD_OPC_Decode, 243, 6, 178, 1, // Opcode: INSVE_D
/* 9067 */    MCD_OPC_FilterValue, 26, 163, 1, // Skip to: 9490
/* 9071 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 9074 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 9087
/* 9078 */    MCD_OPC_CheckPredicate, 8, 36, 18, // Skip to: 13726
/* 9082 */    MCD_OPC_Decode, 178, 5, 139, 1, // Opcode: FCAF_W
/* 9087 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 9100
/* 9091 */    MCD_OPC_CheckPredicate, 8, 23, 18, // Skip to: 13726
/* 9095 */    MCD_OPC_Decode, 177, 5, 140, 1, // Opcode: FCAF_D
/* 9100 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 9113
/* 9104 */    MCD_OPC_CheckPredicate, 8, 10, 18, // Skip to: 13726
/* 9108 */    MCD_OPC_Decode, 205, 5, 139, 1, // Opcode: FCUN_W
/* 9113 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 9126
/* 9117 */    MCD_OPC_CheckPredicate, 8, 253, 17, // Skip to: 13726
/* 9121 */    MCD_OPC_Decode, 204, 5, 140, 1, // Opcode: FCUN_D
/* 9126 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 9139
/* 9130 */    MCD_OPC_CheckPredicate, 8, 240, 17, // Skip to: 13726
/* 9134 */    MCD_OPC_Decode, 180, 5, 139, 1, // Opcode: FCEQ_W
/* 9139 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 9152
/* 9143 */    MCD_OPC_CheckPredicate, 8, 227, 17, // Skip to: 13726
/* 9147 */    MCD_OPC_Decode, 179, 5, 140, 1, // Opcode: FCEQ_D
/* 9152 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 9165
/* 9156 */    MCD_OPC_CheckPredicate, 8, 214, 17, // Skip to: 13726
/* 9160 */    MCD_OPC_Decode, 197, 5, 139, 1, // Opcode: FCUEQ_W
/* 9165 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 9178
/* 9169 */    MCD_OPC_CheckPredicate, 8, 201, 17, // Skip to: 13726
/* 9173 */    MCD_OPC_Decode, 196, 5, 140, 1, // Opcode: FCUEQ_D
/* 9178 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 9191
/* 9182 */    MCD_OPC_CheckPredicate, 8, 188, 17, // Skip to: 13726
/* 9186 */    MCD_OPC_Decode, 186, 5, 139, 1, // Opcode: FCLT_W
/* 9191 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 9204
/* 9195 */    MCD_OPC_CheckPredicate, 8, 175, 17, // Skip to: 13726
/* 9199 */    MCD_OPC_Decode, 185, 5, 140, 1, // Opcode: FCLT_D
/* 9204 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 9217
/* 9208 */    MCD_OPC_CheckPredicate, 8, 162, 17, // Skip to: 13726
/* 9212 */    MCD_OPC_Decode, 201, 5, 139, 1, // Opcode: FCULT_W
/* 9217 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 9230
/* 9221 */    MCD_OPC_CheckPredicate, 8, 149, 17, // Skip to: 13726
/* 9225 */    MCD_OPC_Decode, 200, 5, 140, 1, // Opcode: FCULT_D
/* 9230 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 9243
/* 9234 */    MCD_OPC_CheckPredicate, 8, 136, 17, // Skip to: 13726
/* 9238 */    MCD_OPC_Decode, 184, 5, 139, 1, // Opcode: FCLE_W
/* 9243 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 9256
/* 9247 */    MCD_OPC_CheckPredicate, 8, 123, 17, // Skip to: 13726
/* 9251 */    MCD_OPC_Decode, 183, 5, 140, 1, // Opcode: FCLE_D
/* 9256 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 9269
/* 9260 */    MCD_OPC_CheckPredicate, 8, 110, 17, // Skip to: 13726
/* 9264 */    MCD_OPC_Decode, 199, 5, 139, 1, // Opcode: FCULE_W
/* 9269 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 9282
/* 9273 */    MCD_OPC_CheckPredicate, 8, 97, 17, // Skip to: 13726
/* 9277 */    MCD_OPC_Decode, 198, 5, 140, 1, // Opcode: FCULE_D
/* 9282 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 9295
/* 9286 */    MCD_OPC_CheckPredicate, 8, 84, 17, // Skip to: 13726
/* 9290 */    MCD_OPC_Decode, 154, 6, 139, 1, // Opcode: FSAF_W
/* 9295 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 9308
/* 9299 */    MCD_OPC_CheckPredicate, 8, 71, 17, // Skip to: 13726
/* 9303 */    MCD_OPC_Decode, 153, 6, 140, 1, // Opcode: FSAF_D
/* 9308 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 9321
/* 9312 */    MCD_OPC_CheckPredicate, 8, 58, 17, // Skip to: 13726
/* 9316 */    MCD_OPC_Decode, 188, 6, 139, 1, // Opcode: FSUN_W
/* 9321 */    MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 9334
/* 9325 */    MCD_OPC_CheckPredicate, 8, 45, 17, // Skip to: 13726
/* 9329 */    MCD_OPC_Decode, 187, 6, 140, 1, // Opcode: FSUN_D
/* 9334 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 9347
/* 9338 */    MCD_OPC_CheckPredicate, 8, 32, 17, // Skip to: 13726
/* 9342 */    MCD_OPC_Decode, 156, 6, 139, 1, // Opcode: FSEQ_W
/* 9347 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 9360
/* 9351 */    MCD_OPC_CheckPredicate, 8, 19, 17, // Skip to: 13726
/* 9355 */    MCD_OPC_Decode, 155, 6, 140, 1, // Opcode: FSEQ_D
/* 9360 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 9373
/* 9364 */    MCD_OPC_CheckPredicate, 8, 6, 17, // Skip to: 13726
/* 9368 */    MCD_OPC_Decode, 180, 6, 139, 1, // Opcode: FSUEQ_W
/* 9373 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 9386
/* 9377 */    MCD_OPC_CheckPredicate, 8, 249, 16, // Skip to: 13726
/* 9381 */    MCD_OPC_Decode, 179, 6, 140, 1, // Opcode: FSUEQ_D
/* 9386 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 9399
/* 9390 */    MCD_OPC_CheckPredicate, 8, 236, 16, // Skip to: 13726
/* 9394 */    MCD_OPC_Decode, 160, 6, 139, 1, // Opcode: FSLT_W
/* 9399 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 9412
/* 9403 */    MCD_OPC_CheckPredicate, 8, 223, 16, // Skip to: 13726
/* 9407 */    MCD_OPC_Decode, 159, 6, 140, 1, // Opcode: FSLT_D
/* 9412 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 9425
/* 9416 */    MCD_OPC_CheckPredicate, 8, 210, 16, // Skip to: 13726
/* 9420 */    MCD_OPC_Decode, 184, 6, 139, 1, // Opcode: FSULT_W
/* 9425 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 9438
/* 9429 */    MCD_OPC_CheckPredicate, 8, 197, 16, // Skip to: 13726
/* 9433 */    MCD_OPC_Decode, 183, 6, 140, 1, // Opcode: FSULT_D
/* 9438 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 9451
/* 9442 */    MCD_OPC_CheckPredicate, 8, 184, 16, // Skip to: 13726
/* 9446 */    MCD_OPC_Decode, 158, 6, 139, 1, // Opcode: FSLE_W
/* 9451 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 9464
/* 9455 */    MCD_OPC_CheckPredicate, 8, 171, 16, // Skip to: 13726
/* 9459 */    MCD_OPC_Decode, 157, 6, 140, 1, // Opcode: FSLE_D
/* 9464 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 9477
/* 9468 */    MCD_OPC_CheckPredicate, 8, 158, 16, // Skip to: 13726
/* 9472 */    MCD_OPC_Decode, 182, 6, 139, 1, // Opcode: FSULE_W
/* 9477 */    MCD_OPC_FilterValue, 31, 149, 16, // Skip to: 13726
/* 9481 */    MCD_OPC_CheckPredicate, 8, 145, 16, // Skip to: 13726
/* 9485 */    MCD_OPC_Decode, 181, 6, 140, 1, // Opcode: FSULE_D
/* 9490 */    MCD_OPC_FilterValue, 27, 85, 1, // Skip to: 9835
/* 9494 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 9497 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 9510
/* 9501 */    MCD_OPC_CheckPredicate, 8, 125, 16, // Skip to: 13726
/* 9505 */    MCD_OPC_Decode, 176, 5, 139, 1, // Opcode: FADD_W
/* 9510 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 9523
/* 9514 */    MCD_OPC_CheckPredicate, 8, 112, 16, // Skip to: 13726
/* 9518 */    MCD_OPC_Decode, 170, 5, 140, 1, // Opcode: FADD_D
/* 9523 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 9536
/* 9527 */    MCD_OPC_CheckPredicate, 8, 99, 16, // Skip to: 13726
/* 9531 */    MCD_OPC_Decode, 178, 6, 139, 1, // Opcode: FSUB_W
/* 9536 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 9549
/* 9540 */    MCD_OPC_CheckPredicate, 8, 86, 16, // Skip to: 13726
/* 9544 */    MCD_OPC_Decode, 172, 6, 140, 1, // Opcode: FSUB_D
/* 9549 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 9562
/* 9553 */    MCD_OPC_CheckPredicate, 8, 73, 16, // Skip to: 13726
/* 9557 */    MCD_OPC_Decode, 141, 6, 139, 1, // Opcode: FMUL_W
/* 9562 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 9575
/* 9566 */    MCD_OPC_CheckPredicate, 8, 60, 16, // Skip to: 13726
/* 9570 */    MCD_OPC_Decode, 135, 6, 140, 1, // Opcode: FMUL_D
/* 9575 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 9588
/* 9579 */    MCD_OPC_CheckPredicate, 8, 47, 16, // Skip to: 13726
/* 9583 */    MCD_OPC_Decode, 212, 5, 139, 1, // Opcode: FDIV_W
/* 9588 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 9601
/* 9592 */    MCD_OPC_CheckPredicate, 8, 34, 16, // Skip to: 13726
/* 9596 */    MCD_OPC_Decode, 206, 5, 140, 1, // Opcode: FDIV_D
/* 9601 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 9614
/* 9605 */    MCD_OPC_CheckPredicate, 8, 21, 16, // Skip to: 13726
/* 9609 */    MCD_OPC_Decode, 247, 5, 143, 1, // Opcode: FMADD_W
/* 9614 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 9627
/* 9618 */    MCD_OPC_CheckPredicate, 8, 8, 16, // Skip to: 13726
/* 9622 */    MCD_OPC_Decode, 246, 5, 144, 1, // Opcode: FMADD_D
/* 9627 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 9640
/* 9631 */    MCD_OPC_CheckPredicate, 8, 251, 15, // Skip to: 13726
/* 9635 */    MCD_OPC_Decode, 134, 6, 143, 1, // Opcode: FMSUB_W
/* 9640 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 9653
/* 9644 */    MCD_OPC_CheckPredicate, 8, 238, 15, // Skip to: 13726
/* 9648 */    MCD_OPC_Decode, 133, 6, 144, 1, // Opcode: FMSUB_D
/* 9653 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 9666
/* 9657 */    MCD_OPC_CheckPredicate, 8, 225, 15, // Skip to: 13726
/* 9661 */    MCD_OPC_Decode, 217, 5, 139, 1, // Opcode: FEXP2_W
/* 9666 */    MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 9679
/* 9670 */    MCD_OPC_CheckPredicate, 8, 212, 15, // Skip to: 13726
/* 9674 */    MCD_OPC_Decode, 215, 5, 140, 1, // Opcode: FEXP2_D
/* 9679 */    MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 9692
/* 9683 */    MCD_OPC_CheckPredicate, 8, 199, 15, // Skip to: 13726
/* 9687 */    MCD_OPC_Decode, 213, 5, 179, 1, // Opcode: FEXDO_H
/* 9692 */    MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 9705
/* 9696 */    MCD_OPC_CheckPredicate, 8, 186, 15, // Skip to: 13726
/* 9700 */    MCD_OPC_Decode, 214, 5, 180, 1, // Opcode: FEXDO_W
/* 9705 */    MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 9718
/* 9709 */    MCD_OPC_CheckPredicate, 8, 173, 15, // Skip to: 13726
/* 9713 */    MCD_OPC_Decode, 193, 6, 179, 1, // Opcode: FTQ_H
/* 9718 */    MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 9731
/* 9722 */    MCD_OPC_CheckPredicate, 8, 160, 15, // Skip to: 13726
/* 9726 */    MCD_OPC_Decode, 194, 6, 180, 1, // Opcode: FTQ_W
/* 9731 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 9744
/* 9735 */    MCD_OPC_CheckPredicate, 8, 147, 15, // Skip to: 13726
/* 9739 */    MCD_OPC_Decode, 255, 5, 139, 1, // Opcode: FMIN_W
/* 9744 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 9757
/* 9748 */    MCD_OPC_CheckPredicate, 8, 134, 15, // Skip to: 13726
/* 9752 */    MCD_OPC_Decode, 254, 5, 140, 1, // Opcode: FMIN_D
/* 9757 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 9770
/* 9761 */    MCD_OPC_CheckPredicate, 8, 121, 15, // Skip to: 13726
/* 9765 */    MCD_OPC_Decode, 253, 5, 139, 1, // Opcode: FMIN_A_W
/* 9770 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 9783
/* 9774 */    MCD_OPC_CheckPredicate, 8, 108, 15, // Skip to: 13726
/* 9778 */    MCD_OPC_Decode, 252, 5, 140, 1, // Opcode: FMIN_A_D
/* 9783 */    MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 9796
/* 9787 */    MCD_OPC_CheckPredicate, 8, 95, 15, // Skip to: 13726
/* 9791 */    MCD_OPC_Decode, 251, 5, 139, 1, // Opcode: FMAX_W
/* 9796 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 9809
/* 9800 */    MCD_OPC_CheckPredicate, 8, 82, 15, // Skip to: 13726
/* 9804 */    MCD_OPC_Decode, 250, 5, 140, 1, // Opcode: FMAX_D
/* 9809 */    MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 9822
/* 9813 */    MCD_OPC_CheckPredicate, 8, 69, 15, // Skip to: 13726
/* 9817 */    MCD_OPC_Decode, 249, 5, 139, 1, // Opcode: FMAX_A_W
/* 9822 */    MCD_OPC_FilterValue, 31, 60, 15, // Skip to: 13726
/* 9826 */    MCD_OPC_CheckPredicate, 8, 56, 15, // Skip to: 13726
/* 9830 */    MCD_OPC_Decode, 248, 5, 140, 1, // Opcode: FMAX_A_D
/* 9835 */    MCD_OPC_FilterValue, 28, 59, 1, // Skip to: 10154
/* 9839 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 9842 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 9855
/* 9846 */    MCD_OPC_CheckPredicate, 8, 36, 15, // Skip to: 13726
/* 9850 */    MCD_OPC_Decode, 195, 5, 139, 1, // Opcode: FCOR_W
/* 9855 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 9868
/* 9859 */    MCD_OPC_CheckPredicate, 8, 23, 15, // Skip to: 13726
/* 9863 */    MCD_OPC_Decode, 194, 5, 140, 1, // Opcode: FCOR_D
/* 9868 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 9881
/* 9872 */    MCD_OPC_CheckPredicate, 8, 10, 15, // Skip to: 13726
/* 9876 */    MCD_OPC_Decode, 203, 5, 139, 1, // Opcode: FCUNE_W
/* 9881 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 9894
/* 9885 */    MCD_OPC_CheckPredicate, 8, 253, 14, // Skip to: 13726
/* 9889 */    MCD_OPC_Decode, 202, 5, 140, 1, // Opcode: FCUNE_D
/* 9894 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 9907
/* 9898 */    MCD_OPC_CheckPredicate, 8, 240, 14, // Skip to: 13726
/* 9902 */    MCD_OPC_Decode, 193, 5, 139, 1, // Opcode: FCNE_W
/* 9907 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 9920
/* 9911 */    MCD_OPC_CheckPredicate, 8, 227, 14, // Skip to: 13726
/* 9915 */    MCD_OPC_Decode, 192, 5, 140, 1, // Opcode: FCNE_D
/* 9920 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 9933
/* 9924 */    MCD_OPC_CheckPredicate, 8, 214, 14, // Skip to: 13726
/* 9928 */    MCD_OPC_Decode, 219, 9, 138, 1, // Opcode: MUL_Q_H
/* 9933 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 9946
/* 9937 */    MCD_OPC_CheckPredicate, 8, 201, 14, // Skip to: 13726
/* 9941 */    MCD_OPC_Decode, 220, 9, 139, 1, // Opcode: MUL_Q_W
/* 9946 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 9959
/* 9950 */    MCD_OPC_CheckPredicate, 8, 188, 14, // Skip to: 13726
/* 9954 */    MCD_OPC_Decode, 147, 8, 142, 1, // Opcode: MADD_Q_H
/* 9959 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 9972
/* 9963 */    MCD_OPC_CheckPredicate, 8, 175, 14, // Skip to: 13726
/* 9967 */    MCD_OPC_Decode, 148, 8, 143, 1, // Opcode: MADD_Q_W
/* 9972 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 9985
/* 9976 */    MCD_OPC_CheckPredicate, 8, 162, 14, // Skip to: 13726
/* 9980 */    MCD_OPC_Decode, 165, 9, 142, 1, // Opcode: MSUB_Q_H
/* 9985 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 9998
/* 9989 */    MCD_OPC_CheckPredicate, 8, 149, 14, // Skip to: 13726
/* 9993 */    MCD_OPC_Decode, 166, 9, 143, 1, // Opcode: MSUB_Q_W
/* 9998 */    MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 10011
/* 10002 */   MCD_OPC_CheckPredicate, 8, 136, 14, // Skip to: 13726
/* 10006 */   MCD_OPC_Decode, 164, 6, 139, 1, // Opcode: FSOR_W
/* 10011 */   MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 10024
/* 10015 */   MCD_OPC_CheckPredicate, 8, 123, 14, // Skip to: 13726
/* 10019 */   MCD_OPC_Decode, 163, 6, 140, 1, // Opcode: FSOR_D
/* 10024 */   MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 10037
/* 10028 */   MCD_OPC_CheckPredicate, 8, 110, 14, // Skip to: 13726
/* 10032 */   MCD_OPC_Decode, 186, 6, 139, 1, // Opcode: FSUNE_W
/* 10037 */   MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 10050
/* 10041 */   MCD_OPC_CheckPredicate, 8, 97, 14, // Skip to: 13726
/* 10045 */   MCD_OPC_Decode, 185, 6, 140, 1, // Opcode: FSUNE_D
/* 10050 */   MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 10063
/* 10054 */   MCD_OPC_CheckPredicate, 8, 84, 14, // Skip to: 13726
/* 10058 */   MCD_OPC_Decode, 162, 6, 139, 1, // Opcode: FSNE_W
/* 10063 */   MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 10076
/* 10067 */   MCD_OPC_CheckPredicate, 8, 71, 14, // Skip to: 13726
/* 10071 */   MCD_OPC_Decode, 161, 6, 140, 1, // Opcode: FSNE_D
/* 10076 */   MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 10089
/* 10080 */   MCD_OPC_CheckPredicate, 8, 58, 14, // Skip to: 13726
/* 10084 */   MCD_OPC_Decode, 202, 9, 138, 1, // Opcode: MULR_Q_H
/* 10089 */   MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 10102
/* 10093 */   MCD_OPC_CheckPredicate, 8, 45, 14, // Skip to: 13726
/* 10097 */   MCD_OPC_Decode, 203, 9, 139, 1, // Opcode: MULR_Q_W
/* 10102 */   MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 10115
/* 10106 */   MCD_OPC_CheckPredicate, 8, 32, 14, // Skip to: 13726
/* 10110 */   MCD_OPC_Decode, 133, 8, 142, 1, // Opcode: MADDR_Q_H
/* 10115 */   MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 10128
/* 10119 */   MCD_OPC_CheckPredicate, 8, 19, 14, // Skip to: 13726
/* 10123 */   MCD_OPC_Decode, 134, 8, 143, 1, // Opcode: MADDR_Q_W
/* 10128 */   MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 10141
/* 10132 */   MCD_OPC_CheckPredicate, 8, 6, 14, // Skip to: 13726
/* 10136 */   MCD_OPC_Decode, 151, 9, 142, 1, // Opcode: MSUBR_Q_H
/* 10141 */   MCD_OPC_FilterValue, 29, 253, 13, // Skip to: 13726
/* 10145 */   MCD_OPC_CheckPredicate, 8, 249, 13, // Skip to: 13726
/* 10149 */   MCD_OPC_Decode, 152, 9, 143, 1, // Opcode: MSUBR_Q_W
/* 10154 */   MCD_OPC_FilterValue, 30, 219, 2, // Skip to: 10889
/* 10158 */   MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 10161 */   MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 10173
/* 10165 */   MCD_OPC_CheckPredicate, 8, 229, 13, // Skip to: 13726
/* 10169 */   MCD_OPC_Decode, 89, 137, 1, // Opcode: AND_V
/* 10173 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 10186
/* 10177 */   MCD_OPC_CheckPredicate, 8, 217, 13, // Skip to: 13726
/* 10181 */   MCD_OPC_Decode, 138, 10, 137, 1, // Opcode: OR_V
/* 10186 */   MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 10199
/* 10190 */   MCD_OPC_CheckPredicate, 8, 204, 13, // Skip to: 13726
/* 10194 */   MCD_OPC_Decode, 254, 9, 137, 1, // Opcode: NOR_V
/* 10199 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 10212
/* 10203 */   MCD_OPC_CheckPredicate, 8, 191, 13, // Skip to: 13726
/* 10207 */   MCD_OPC_Decode, 241, 13, 137, 1, // Opcode: XOR_V
/* 10212 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 10225
/* 10216 */   MCD_OPC_CheckPredicate, 8, 178, 13, // Skip to: 13726
/* 10220 */   MCD_OPC_Decode, 142, 2, 141, 1, // Opcode: BMNZ_V
/* 10225 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 10238
/* 10229 */   MCD_OPC_CheckPredicate, 8, 165, 13, // Skip to: 13726
/* 10233 */   MCD_OPC_Decode, 144, 2, 141, 1, // Opcode: BMZ_V
/* 10238 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 10251
/* 10242 */   MCD_OPC_CheckPredicate, 8, 152, 13, // Skip to: 13726
/* 10246 */   MCD_OPC_Decode, 179, 2, 141, 1, // Opcode: BSEL_V
/* 10251 */   MCD_OPC_FilterValue, 24, 211, 0, // Skip to: 10466
/* 10255 */   MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 10258 */   MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 10271
/* 10262 */   MCD_OPC_CheckPredicate, 8, 132, 13, // Skip to: 13726
/* 10266 */   MCD_OPC_Decode, 231, 5, 181, 1, // Opcode: FILL_B
/* 10271 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 10284
/* 10275 */   MCD_OPC_CheckPredicate, 8, 119, 13, // Skip to: 13726
/* 10279 */   MCD_OPC_Decode, 235, 5, 182, 1, // Opcode: FILL_H
/* 10284 */   MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 10297
/* 10288 */   MCD_OPC_CheckPredicate, 8, 106, 13, // Skip to: 13726
/* 10292 */   MCD_OPC_Decode, 236, 5, 183, 1, // Opcode: FILL_W
/* 10297 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 10310
/* 10301 */   MCD_OPC_CheckPredicate, 14, 93, 13, // Skip to: 13726
/* 10305 */   MCD_OPC_Decode, 232, 5, 184, 1, // Opcode: FILL_D
/* 10310 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 10323
/* 10314 */   MCD_OPC_CheckPredicate, 8, 80, 13, // Skip to: 13726
/* 10318 */   MCD_OPC_Decode, 157, 10, 173, 1, // Opcode: PCNT_B
/* 10323 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 10336
/* 10327 */   MCD_OPC_CheckPredicate, 8, 67, 13, // Skip to: 13726
/* 10331 */   MCD_OPC_Decode, 159, 10, 185, 1, // Opcode: PCNT_H
/* 10336 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 10349
/* 10340 */   MCD_OPC_CheckPredicate, 8, 54, 13, // Skip to: 13726
/* 10344 */   MCD_OPC_Decode, 160, 10, 186, 1, // Opcode: PCNT_W
/* 10349 */   MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 10362
/* 10353 */   MCD_OPC_CheckPredicate, 8, 41, 13, // Skip to: 13726
/* 10357 */   MCD_OPC_Decode, 158, 10, 187, 1, // Opcode: PCNT_D
/* 10362 */   MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 10375
/* 10366 */   MCD_OPC_CheckPredicate, 8, 28, 13, // Skip to: 13726
/* 10370 */   MCD_OPC_Decode, 231, 9, 173, 1, // Opcode: NLOC_B
/* 10375 */   MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 10388
/* 10379 */   MCD_OPC_CheckPredicate, 8, 15, 13, // Skip to: 13726
/* 10383 */   MCD_OPC_Decode, 233, 9, 185, 1, // Opcode: NLOC_H
/* 10388 */   MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 10401
/* 10392 */   MCD_OPC_CheckPredicate, 8, 2, 13, // Skip to: 13726
/* 10396 */   MCD_OPC_Decode, 234, 9, 186, 1, // Opcode: NLOC_W
/* 10401 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 10414
/* 10405 */   MCD_OPC_CheckPredicate, 8, 245, 12, // Skip to: 13726
/* 10409 */   MCD_OPC_Decode, 232, 9, 187, 1, // Opcode: NLOC_D
/* 10414 */   MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 10427
/* 10418 */   MCD_OPC_CheckPredicate, 8, 232, 12, // Skip to: 13726
/* 10422 */   MCD_OPC_Decode, 235, 9, 173, 1, // Opcode: NLZC_B
/* 10427 */   MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 10440
/* 10431 */   MCD_OPC_CheckPredicate, 8, 219, 12, // Skip to: 13726
/* 10435 */   MCD_OPC_Decode, 237, 9, 185, 1, // Opcode: NLZC_H
/* 10440 */   MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 10453
/* 10444 */   MCD_OPC_CheckPredicate, 8, 206, 12, // Skip to: 13726
/* 10448 */   MCD_OPC_Decode, 238, 9, 186, 1, // Opcode: NLZC_W
/* 10453 */   MCD_OPC_FilterValue, 15, 197, 12, // Skip to: 13726
/* 10457 */   MCD_OPC_CheckPredicate, 8, 193, 12, // Skip to: 13726
/* 10461 */   MCD_OPC_Decode, 236, 9, 187, 1, // Opcode: NLZC_D
/* 10466 */   MCD_OPC_FilterValue, 25, 184, 12, // Skip to: 13726
/* 10470 */   MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 10473 */   MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 10486
/* 10477 */   MCD_OPC_CheckPredicate, 8, 173, 12, // Skip to: 13726
/* 10481 */   MCD_OPC_Decode, 182, 5, 186, 1, // Opcode: FCLASS_W
/* 10486 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 10499
/* 10490 */   MCD_OPC_CheckPredicate, 8, 160, 12, // Skip to: 13726
/* 10494 */   MCD_OPC_Decode, 181, 5, 187, 1, // Opcode: FCLASS_D
/* 10499 */   MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 10512
/* 10503 */   MCD_OPC_CheckPredicate, 8, 147, 12, // Skip to: 13726
/* 10507 */   MCD_OPC_Decode, 196, 6, 186, 1, // Opcode: FTRUNC_S_W
/* 10512 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 10525
/* 10516 */   MCD_OPC_CheckPredicate, 8, 134, 12, // Skip to: 13726
/* 10520 */   MCD_OPC_Decode, 195, 6, 187, 1, // Opcode: FTRUNC_S_D
/* 10525 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 10538
/* 10529 */   MCD_OPC_CheckPredicate, 8, 121, 12, // Skip to: 13726
/* 10533 */   MCD_OPC_Decode, 198, 6, 186, 1, // Opcode: FTRUNC_U_W
/* 10538 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 10551
/* 10542 */   MCD_OPC_CheckPredicate, 8, 108, 12, // Skip to: 13726
/* 10546 */   MCD_OPC_Decode, 197, 6, 187, 1, // Opcode: FTRUNC_U_D
/* 10551 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 10564
/* 10555 */   MCD_OPC_CheckPredicate, 8, 95, 12, // Skip to: 13726
/* 10559 */   MCD_OPC_Decode, 171, 6, 186, 1, // Opcode: FSQRT_W
/* 10564 */   MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 10577
/* 10568 */   MCD_OPC_CheckPredicate, 8, 82, 12, // Skip to: 13726
/* 10572 */   MCD_OPC_Decode, 165, 6, 187, 1, // Opcode: FSQRT_D
/* 10577 */   MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 10590
/* 10581 */   MCD_OPC_CheckPredicate, 8, 69, 12, // Skip to: 13726
/* 10585 */   MCD_OPC_Decode, 152, 6, 186, 1, // Opcode: FRSQRT_W
/* 10590 */   MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 10603
/* 10594 */   MCD_OPC_CheckPredicate, 8, 56, 12, // Skip to: 13726
/* 10598 */   MCD_OPC_Decode, 151, 6, 187, 1, // Opcode: FRSQRT_D
/* 10603 */   MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 10616
/* 10607 */   MCD_OPC_CheckPredicate, 8, 43, 12, // Skip to: 13726
/* 10611 */   MCD_OPC_Decode, 148, 6, 186, 1, // Opcode: FRCP_W
/* 10616 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 10629
/* 10620 */   MCD_OPC_CheckPredicate, 8, 30, 12, // Skip to: 13726
/* 10624 */   MCD_OPC_Decode, 147, 6, 187, 1, // Opcode: FRCP_D
/* 10629 */   MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 10642
/* 10633 */   MCD_OPC_CheckPredicate, 8, 17, 12, // Skip to: 13726
/* 10637 */   MCD_OPC_Decode, 150, 6, 186, 1, // Opcode: FRINT_W
/* 10642 */   MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 10655
/* 10646 */   MCD_OPC_CheckPredicate, 8, 4, 12, // Skip to: 13726
/* 10650 */   MCD_OPC_Decode, 149, 6, 187, 1, // Opcode: FRINT_D
/* 10655 */   MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 10668
/* 10659 */   MCD_OPC_CheckPredicate, 8, 247, 11, // Skip to: 13726
/* 10663 */   MCD_OPC_Decode, 238, 5, 186, 1, // Opcode: FLOG2_W
/* 10668 */   MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 10681
/* 10672 */   MCD_OPC_CheckPredicate, 8, 234, 11, // Skip to: 13726
/* 10676 */   MCD_OPC_Decode, 237, 5, 187, 1, // Opcode: FLOG2_D
/* 10681 */   MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 10694
/* 10685 */   MCD_OPC_CheckPredicate, 8, 221, 11, // Skip to: 13726
/* 10689 */   MCD_OPC_Decode, 220, 5, 188, 1, // Opcode: FEXUPL_W
/* 10694 */   MCD_OPC_FilterValue, 17, 9, 0, // Skip to: 10707
/* 10698 */   MCD_OPC_CheckPredicate, 8, 208, 11, // Skip to: 13726
/* 10702 */   MCD_OPC_Decode, 219, 5, 189, 1, // Opcode: FEXUPL_D
/* 10707 */   MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 10720
/* 10711 */   MCD_OPC_CheckPredicate, 8, 195, 11, // Skip to: 13726
/* 10715 */   MCD_OPC_Decode, 222, 5, 188, 1, // Opcode: FEXUPR_W
/* 10720 */   MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 10733
/* 10724 */   MCD_OPC_CheckPredicate, 8, 182, 11, // Skip to: 13726
/* 10728 */   MCD_OPC_Decode, 221, 5, 189, 1, // Opcode: FEXUPR_D
/* 10733 */   MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 10746
/* 10737 */   MCD_OPC_CheckPredicate, 8, 169, 11, // Skip to: 13726
/* 10741 */   MCD_OPC_Decode, 228, 5, 188, 1, // Opcode: FFQL_W
/* 10746 */   MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 10759
/* 10750 */   MCD_OPC_CheckPredicate, 8, 156, 11, // Skip to: 13726
/* 10754 */   MCD_OPC_Decode, 227, 5, 189, 1, // Opcode: FFQL_D
/* 10759 */   MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 10772
/* 10763 */   MCD_OPC_CheckPredicate, 8, 143, 11, // Skip to: 13726
/* 10767 */   MCD_OPC_Decode, 230, 5, 188, 1, // Opcode: FFQR_W
/* 10772 */   MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 10785
/* 10776 */   MCD_OPC_CheckPredicate, 8, 130, 11, // Skip to: 13726
/* 10780 */   MCD_OPC_Decode, 229, 5, 189, 1, // Opcode: FFQR_D
/* 10785 */   MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 10798
/* 10789 */   MCD_OPC_CheckPredicate, 8, 117, 11, // Skip to: 13726
/* 10793 */   MCD_OPC_Decode, 190, 6, 186, 1, // Opcode: FTINT_S_W
/* 10798 */   MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 10811
/* 10802 */   MCD_OPC_CheckPredicate, 8, 104, 11, // Skip to: 13726
/* 10806 */   MCD_OPC_Decode, 189, 6, 187, 1, // Opcode: FTINT_S_D
/* 10811 */   MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 10824
/* 10815 */   MCD_OPC_CheckPredicate, 8, 91, 11, // Skip to: 13726
/* 10819 */   MCD_OPC_Decode, 192, 6, 186, 1, // Opcode: FTINT_U_W
/* 10824 */   MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 10837
/* 10828 */   MCD_OPC_CheckPredicate, 8, 78, 11, // Skip to: 13726
/* 10832 */   MCD_OPC_Decode, 191, 6, 187, 1, // Opcode: FTINT_U_D
/* 10837 */   MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 10850
/* 10841 */   MCD_OPC_CheckPredicate, 8, 65, 11, // Skip to: 13726
/* 10845 */   MCD_OPC_Decode, 224, 5, 186, 1, // Opcode: FFINT_S_W
/* 10850 */   MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 10863
/* 10854 */   MCD_OPC_CheckPredicate, 8, 52, 11, // Skip to: 13726
/* 10858 */   MCD_OPC_Decode, 223, 5, 187, 1, // Opcode: FFINT_S_D
/* 10863 */   MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 10876
/* 10867 */   MCD_OPC_CheckPredicate, 8, 39, 11, // Skip to: 13726
/* 10871 */   MCD_OPC_Decode, 226, 5, 186, 1, // Opcode: FFINT_U_W
/* 10876 */   MCD_OPC_FilterValue, 31, 30, 11, // Skip to: 13726
/* 10880 */   MCD_OPC_CheckPredicate, 8, 26, 11, // Skip to: 13726
/* 10884 */   MCD_OPC_Decode, 225, 5, 187, 1, // Opcode: FFINT_U_D
/* 10889 */   MCD_OPC_FilterValue, 32, 9, 0, // Skip to: 10902
/* 10893 */   MCD_OPC_CheckPredicate, 8, 13, 11, // Skip to: 13726
/* 10897 */   MCD_OPC_Decode, 177, 7, 190, 1, // Opcode: LD_B
/* 10902 */   MCD_OPC_FilterValue, 33, 9, 0, // Skip to: 10915
/* 10906 */   MCD_OPC_CheckPredicate, 8, 0, 11, // Skip to: 13726
/* 10910 */   MCD_OPC_Decode, 179, 7, 190, 1, // Opcode: LD_H
/* 10915 */   MCD_OPC_FilterValue, 34, 9, 0, // Skip to: 10928
/* 10919 */   MCD_OPC_CheckPredicate, 8, 243, 10, // Skip to: 13726
/* 10923 */   MCD_OPC_Decode, 180, 7, 190, 1, // Opcode: LD_W
/* 10928 */   MCD_OPC_FilterValue, 35, 9, 0, // Skip to: 10941
/* 10932 */   MCD_OPC_CheckPredicate, 8, 230, 10, // Skip to: 13726
/* 10936 */   MCD_OPC_Decode, 178, 7, 190, 1, // Opcode: LD_D
/* 10941 */   MCD_OPC_FilterValue, 36, 9, 0, // Skip to: 10954
/* 10945 */   MCD_OPC_CheckPredicate, 8, 217, 10, // Skip to: 13726
/* 10949 */   MCD_OPC_Decode, 186, 12, 190, 1, // Opcode: ST_B
/* 10954 */   MCD_OPC_FilterValue, 37, 9, 0, // Skip to: 10967
/* 10958 */   MCD_OPC_CheckPredicate, 8, 204, 10, // Skip to: 13726
/* 10962 */   MCD_OPC_Decode, 188, 12, 190, 1, // Opcode: ST_H
/* 10967 */   MCD_OPC_FilterValue, 38, 9, 0, // Skip to: 10980
/* 10971 */   MCD_OPC_CheckPredicate, 8, 191, 10, // Skip to: 13726
/* 10975 */   MCD_OPC_Decode, 189, 12, 190, 1, // Opcode: ST_W
/* 10980 */   MCD_OPC_FilterValue, 39, 182, 10, // Skip to: 13726
/* 10984 */   MCD_OPC_CheckPredicate, 8, 178, 10, // Skip to: 13726
/* 10988 */   MCD_OPC_Decode, 187, 12, 190, 1, // Opcode: ST_D
/* 10993 */   MCD_OPC_FilterValue, 31, 113, 9, // Skip to: 13414
/* 10997 */   MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 11000 */   MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 11013
/* 11004 */   MCD_OPC_CheckPredicate, 6, 158, 10, // Skip to: 13726
/* 11008 */   MCD_OPC_Decode, 145, 5, 191, 1, // Opcode: EXT
/* 11013 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 11026
/* 11017 */   MCD_OPC_CheckPredicate, 6, 145, 10, // Skip to: 13726
/* 11021 */   MCD_OPC_Decode, 228, 6, 192, 1, // Opcode: INS
/* 11026 */   MCD_OPC_FilterValue, 10, 42, 0, // Skip to: 11072
/* 11030 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 11033 */   MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 11046
/* 11037 */   MCD_OPC_CheckPredicate, 12, 125, 10, // Skip to: 13726
/* 11041 */   MCD_OPC_Decode, 236, 7, 193, 1, // Opcode: LWX
/* 11046 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 11059
/* 11050 */   MCD_OPC_CheckPredicate, 12, 112, 10, // Skip to: 13726
/* 11054 */   MCD_OPC_Decode, 187, 7, 193, 1, // Opcode: LHX
/* 11059 */   MCD_OPC_FilterValue, 6, 103, 10, // Skip to: 13726
/* 11063 */   MCD_OPC_CheckPredicate, 12, 99, 10, // Skip to: 13726
/* 11067 */   MCD_OPC_Decode, 156, 7, 193, 1, // Opcode: LBUX
/* 11072 */   MCD_OPC_FilterValue, 12, 15, 0, // Skip to: 11091
/* 11076 */   MCD_OPC_CheckPredicate, 12, 86, 10, // Skip to: 13726
/* 11080 */   MCD_OPC_CheckField, 6, 10, 0, 80, 10, // Skip to: 13726
/* 11086 */   MCD_OPC_Decode, 241, 6, 194, 1, // Opcode: INSV
/* 11091 */   MCD_OPC_FilterValue, 16, 51, 1, // Skip to: 11402
/* 11095 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 11098 */   MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 11110
/* 11102 */   MCD_OPC_CheckPredicate, 12, 60, 10, // Skip to: 13726
/* 11106 */   MCD_OPC_Decode, 56, 195, 1, // Opcode: ADDU_QB
/* 11110 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 11123
/* 11114 */   MCD_OPC_CheckPredicate, 12, 48, 10, // Skip to: 13726
/* 11118 */   MCD_OPC_Decode, 218, 12, 195, 1, // Opcode: SUBU_QB
/* 11123 */   MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 11135
/* 11127 */   MCD_OPC_CheckPredicate, 12, 35, 10, // Skip to: 13726
/* 11131 */   MCD_OPC_Decode, 58, 195, 1, // Opcode: ADDU_S_QB
/* 11135 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 11148
/* 11139 */   MCD_OPC_CheckPredicate, 12, 23, 10, // Skip to: 13726
/* 11143 */   MCD_OPC_Decode, 220, 12, 195, 1, // Opcode: SUBU_S_QB
/* 11148 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 11161
/* 11152 */   MCD_OPC_CheckPredicate, 12, 10, 10, // Skip to: 13726
/* 11156 */   MCD_OPC_Decode, 196, 9, 195, 1, // Opcode: MULEU_S_PH_QBL
/* 11161 */   MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 11174
/* 11165 */   MCD_OPC_CheckPredicate, 12, 253, 9, // Skip to: 13726
/* 11169 */   MCD_OPC_Decode, 197, 9, 195, 1, // Opcode: MULEU_S_PH_QBR
/* 11174 */   MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 11186
/* 11178 */   MCD_OPC_CheckPredicate, 30, 240, 9, // Skip to: 13726
/* 11182 */   MCD_OPC_Decode, 55, 195, 1, // Opcode: ADDU_PH
/* 11186 */   MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 11199
/* 11190 */   MCD_OPC_CheckPredicate, 30, 228, 9, // Skip to: 13726
/* 11194 */   MCD_OPC_Decode, 217, 12, 195, 1, // Opcode: SUBU_PH
/* 11199 */   MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 11211
/* 11203 */   MCD_OPC_CheckPredicate, 12, 215, 9, // Skip to: 13726
/* 11207 */   MCD_OPC_Decode, 36, 195, 1, // Opcode: ADDQ_PH
/* 11211 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 11224
/* 11215 */   MCD_OPC_CheckPredicate, 12, 203, 9, // Skip to: 13726
/* 11219 */   MCD_OPC_Decode, 195, 12, 195, 1, // Opcode: SUBQ_PH
/* 11224 */   MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 11236
/* 11228 */   MCD_OPC_CheckPredicate, 30, 190, 9, // Skip to: 13726
/* 11232 */   MCD_OPC_Decode, 57, 195, 1, // Opcode: ADDU_S_PH
/* 11236 */   MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 11249
/* 11240 */   MCD_OPC_CheckPredicate, 30, 178, 9, // Skip to: 13726
/* 11244 */   MCD_OPC_Decode, 219, 12, 195, 1, // Opcode: SUBU_S_PH
/* 11249 */   MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 11261
/* 11253 */   MCD_OPC_CheckPredicate, 12, 165, 9, // Skip to: 13726
/* 11257 */   MCD_OPC_Decode, 37, 195, 1, // Opcode: ADDQ_S_PH
/* 11261 */   MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 11274
/* 11265 */   MCD_OPC_CheckPredicate, 12, 153, 9, // Skip to: 13726
/* 11269 */   MCD_OPC_Decode, 196, 12, 195, 1, // Opcode: SUBQ_S_PH
/* 11274 */   MCD_OPC_FilterValue, 16, 7, 0, // Skip to: 11285
/* 11278 */   MCD_OPC_CheckPredicate, 12, 140, 9, // Skip to: 13726
/* 11282 */   MCD_OPC_Decode, 39, 35, // Opcode: ADDSC
/* 11285 */   MCD_OPC_FilterValue, 17, 7, 0, // Skip to: 11296
/* 11289 */   MCD_OPC_CheckPredicate, 12, 129, 9, // Skip to: 13726
/* 11293 */   MCD_OPC_Decode, 67, 35, // Opcode: ADDWC
/* 11296 */   MCD_OPC_FilterValue, 18, 8, 0, // Skip to: 11308
/* 11300 */   MCD_OPC_CheckPredicate, 12, 118, 9, // Skip to: 13726
/* 11304 */   MCD_OPC_Decode, 223, 8, 35, // Opcode: MODSUB
/* 11308 */   MCD_OPC_FilterValue, 20, 15, 0, // Skip to: 11327
/* 11312 */   MCD_OPC_CheckPredicate, 12, 106, 9, // Skip to: 13726
/* 11316 */   MCD_OPC_CheckField, 16, 5, 0, 100, 9, // Skip to: 13726
/* 11322 */   MCD_OPC_Decode, 236, 10, 196, 1, // Opcode: RADDU_W_QB
/* 11327 */   MCD_OPC_FilterValue, 22, 7, 0, // Skip to: 11338
/* 11331 */   MCD_OPC_CheckPredicate, 12, 87, 9, // Skip to: 13726
/* 11335 */   MCD_OPC_Decode, 38, 35, // Opcode: ADDQ_S_W
/* 11338 */   MCD_OPC_FilterValue, 23, 8, 0, // Skip to: 11350
/* 11342 */   MCD_OPC_CheckPredicate, 12, 76, 9, // Skip to: 13726
/* 11346 */   MCD_OPC_Decode, 197, 12, 35, // Opcode: SUBQ_S_W
/* 11350 */   MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 11363
/* 11354 */   MCD_OPC_CheckPredicate, 12, 64, 9, // Skip to: 13726
/* 11358 */   MCD_OPC_Decode, 194, 9, 197, 1, // Opcode: MULEQ_S_W_PHL
/* 11363 */   MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 11376
/* 11367 */   MCD_OPC_CheckPredicate, 12, 51, 9, // Skip to: 13726
/* 11371 */   MCD_OPC_Decode, 195, 9, 197, 1, // Opcode: MULEQ_S_W_PHR
/* 11376 */   MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 11389
/* 11380 */   MCD_OPC_CheckPredicate, 30, 38, 9, // Skip to: 13726
/* 11384 */   MCD_OPC_Decode, 200, 9, 195, 1, // Opcode: MULQ_S_PH
/* 11389 */   MCD_OPC_FilterValue, 31, 29, 9, // Skip to: 13726
/* 11393 */   MCD_OPC_CheckPredicate, 12, 25, 9, // Skip to: 13726
/* 11397 */   MCD_OPC_Decode, 198, 9, 195, 1, // Opcode: MULQ_RS_PH
/* 11402 */   MCD_OPC_FilterValue, 17, 69, 1, // Skip to: 11731
/* 11406 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 11409 */   MCD_OPC_FilterValue, 0, 15, 0, // Skip to: 11428
/* 11413 */   MCD_OPC_CheckPredicate, 12, 5, 9, // Skip to: 13726
/* 11417 */   MCD_OPC_CheckField, 11, 5, 0, 255, 8, // Skip to: 13726
/* 11423 */   MCD_OPC_Decode, 161, 3, 198, 1, // Opcode: CMPU_EQ_QB
/* 11428 */   MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 11447
/* 11432 */   MCD_OPC_CheckPredicate, 12, 242, 8, // Skip to: 13726
/* 11436 */   MCD_OPC_CheckField, 11, 5, 0, 236, 8, // Skip to: 13726
/* 11442 */   MCD_OPC_Decode, 163, 3, 198, 1, // Opcode: CMPU_LT_QB
/* 11447 */   MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 11466
/* 11451 */   MCD_OPC_CheckPredicate, 12, 223, 8, // Skip to: 13726
/* 11455 */   MCD_OPC_CheckField, 11, 5, 0, 217, 8, // Skip to: 13726
/* 11461 */   MCD_OPC_Decode, 162, 3, 198, 1, // Opcode: CMPU_LE_QB
/* 11466 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 11479
/* 11470 */   MCD_OPC_CheckPredicate, 12, 204, 8, // Skip to: 13726
/* 11474 */   MCD_OPC_Decode, 162, 10, 195, 1, // Opcode: PICK_QB
/* 11479 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 11492
/* 11483 */   MCD_OPC_CheckPredicate, 12, 191, 8, // Skip to: 13726
/* 11487 */   MCD_OPC_Decode, 158, 3, 197, 1, // Opcode: CMPGU_EQ_QB
/* 11492 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 11505
/* 11496 */   MCD_OPC_CheckPredicate, 12, 178, 8, // Skip to: 13726
/* 11500 */   MCD_OPC_Decode, 160, 3, 197, 1, // Opcode: CMPGU_LT_QB
/* 11505 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 11518
/* 11509 */   MCD_OPC_CheckPredicate, 12, 165, 8, // Skip to: 13726
/* 11513 */   MCD_OPC_Decode, 159, 3, 197, 1, // Opcode: CMPGU_LE_QB
/* 11518 */   MCD_OPC_FilterValue, 8, 15, 0, // Skip to: 11537
/* 11522 */   MCD_OPC_CheckPredicate, 12, 152, 8, // Skip to: 13726
/* 11526 */   MCD_OPC_CheckField, 11, 5, 0, 146, 8, // Skip to: 13726
/* 11532 */   MCD_OPC_Decode, 165, 3, 198, 1, // Opcode: CMP_EQ_PH
/* 11537 */   MCD_OPC_FilterValue, 9, 15, 0, // Skip to: 11556
/* 11541 */   MCD_OPC_CheckPredicate, 12, 133, 8, // Skip to: 13726
/* 11545 */   MCD_OPC_CheckField, 11, 5, 0, 127, 8, // Skip to: 13726
/* 11551 */   MCD_OPC_Decode, 173, 3, 198, 1, // Opcode: CMP_LT_PH
/* 11556 */   MCD_OPC_FilterValue, 10, 15, 0, // Skip to: 11575
/* 11560 */   MCD_OPC_CheckPredicate, 12, 114, 8, // Skip to: 13726
/* 11564 */   MCD_OPC_CheckField, 11, 5, 0, 108, 8, // Skip to: 13726
/* 11570 */   MCD_OPC_Decode, 170, 3, 198, 1, // Opcode: CMP_LE_PH
/* 11575 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 11588
/* 11579 */   MCD_OPC_CheckPredicate, 12, 95, 8, // Skip to: 13726
/* 11583 */   MCD_OPC_Decode, 161, 10, 195, 1, // Opcode: PICK_PH
/* 11588 */   MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 11601
/* 11592 */   MCD_OPC_CheckPredicate, 12, 82, 8, // Skip to: 13726
/* 11596 */   MCD_OPC_Decode, 176, 10, 195, 1, // Opcode: PRECRQ_QB_PH
/* 11601 */   MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 11614
/* 11605 */   MCD_OPC_CheckPredicate, 30, 69, 8, // Skip to: 13726
/* 11609 */   MCD_OPC_Decode, 178, 10, 195, 1, // Opcode: PRECR_QB_PH
/* 11614 */   MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 11627
/* 11618 */   MCD_OPC_CheckPredicate, 12, 56, 8, // Skip to: 13726
/* 11622 */   MCD_OPC_Decode, 146, 10, 195, 1, // Opcode: PACKRL_PH
/* 11627 */   MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 11640
/* 11631 */   MCD_OPC_CheckPredicate, 12, 43, 8, // Skip to: 13726
/* 11635 */   MCD_OPC_Decode, 174, 10, 195, 1, // Opcode: PRECRQU_S_QB_PH
/* 11640 */   MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 11653
/* 11644 */   MCD_OPC_CheckPredicate, 12, 30, 8, // Skip to: 13726
/* 11648 */   MCD_OPC_Decode, 175, 10, 199, 1, // Opcode: PRECRQ_PH_W
/* 11653 */   MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 11666
/* 11657 */   MCD_OPC_CheckPredicate, 12, 17, 8, // Skip to: 13726
/* 11661 */   MCD_OPC_Decode, 177, 10, 199, 1, // Opcode: PRECRQ_RS_PH_W
/* 11666 */   MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 11679
/* 11670 */   MCD_OPC_CheckPredicate, 30, 4, 8, // Skip to: 13726
/* 11674 */   MCD_OPC_Decode, 155, 3, 197, 1, // Opcode: CMPGDU_EQ_QB
/* 11679 */   MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 11692
/* 11683 */   MCD_OPC_CheckPredicate, 30, 247, 7, // Skip to: 13726
/* 11687 */   MCD_OPC_Decode, 157, 3, 197, 1, // Opcode: CMPGDU_LT_QB
/* 11692 */   MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 11705
/* 11696 */   MCD_OPC_CheckPredicate, 30, 234, 7, // Skip to: 13726
/* 11700 */   MCD_OPC_Decode, 156, 3, 197, 1, // Opcode: CMPGDU_LE_QB
/* 11705 */   MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 11718
/* 11709 */   MCD_OPC_CheckPredicate, 30, 221, 7, // Skip to: 13726
/* 11713 */   MCD_OPC_Decode, 179, 10, 200, 1, // Opcode: PRECR_SRA_PH_W
/* 11718 */   MCD_OPC_FilterValue, 31, 212, 7, // Skip to: 13726
/* 11722 */   MCD_OPC_CheckPredicate, 30, 208, 7, // Skip to: 13726
/* 11726 */   MCD_OPC_Decode, 180, 10, 200, 1, // Opcode: PRECR_SRA_R_PH_W
/* 11731 */   MCD_OPC_FilterValue, 18, 74, 1, // Skip to: 12065
/* 11735 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 11738 */   MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 11756
/* 11742 */   MCD_OPC_CheckPredicate, 30, 188, 7, // Skip to: 13726
/* 11746 */   MCD_OPC_CheckField, 21, 5, 0, 182, 7, // Skip to: 13726
/* 11752 */   MCD_OPC_Decode, 23, 201, 1, // Opcode: ABSQ_S_QB
/* 11756 */   MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 11769
/* 11760 */   MCD_OPC_CheckPredicate, 12, 170, 7, // Skip to: 13726
/* 11764 */   MCD_OPC_Decode, 244, 10, 202, 1, // Opcode: REPL_QB
/* 11769 */   MCD_OPC_FilterValue, 3, 15, 0, // Skip to: 11788
/* 11773 */   MCD_OPC_CheckPredicate, 12, 157, 7, // Skip to: 13726
/* 11777 */   MCD_OPC_CheckField, 21, 5, 0, 151, 7, // Skip to: 13726
/* 11783 */   MCD_OPC_Decode, 242, 10, 203, 1, // Opcode: REPLV_QB
/* 11788 */   MCD_OPC_FilterValue, 4, 15, 0, // Skip to: 11807
/* 11792 */   MCD_OPC_CheckPredicate, 12, 138, 7, // Skip to: 13726
/* 11796 */   MCD_OPC_CheckField, 21, 5, 0, 132, 7, // Skip to: 13726
/* 11802 */   MCD_OPC_Decode, 164, 10, 201, 1, // Opcode: PRECEQU_PH_QBL
/* 11807 */   MCD_OPC_FilterValue, 5, 15, 0, // Skip to: 11826
/* 11811 */   MCD_OPC_CheckPredicate, 12, 119, 7, // Skip to: 13726
/* 11815 */   MCD_OPC_CheckField, 21, 5, 0, 113, 7, // Skip to: 13726
/* 11821 */   MCD_OPC_Decode, 166, 10, 201, 1, // Opcode: PRECEQU_PH_QBR
/* 11826 */   MCD_OPC_FilterValue, 6, 15, 0, // Skip to: 11845
/* 11830 */   MCD_OPC_CheckPredicate, 12, 100, 7, // Skip to: 13726
/* 11834 */   MCD_OPC_CheckField, 21, 5, 0, 94, 7, // Skip to: 13726
/* 11840 */   MCD_OPC_Decode, 165, 10, 201, 1, // Opcode: PRECEQU_PH_QBLA
/* 11845 */   MCD_OPC_FilterValue, 7, 15, 0, // Skip to: 11864
/* 11849 */   MCD_OPC_CheckPredicate, 12, 81, 7, // Skip to: 13726
/* 11853 */   MCD_OPC_CheckField, 21, 5, 0, 75, 7, // Skip to: 13726
/* 11859 */   MCD_OPC_Decode, 167, 10, 201, 1, // Opcode: PRECEQU_PH_QBRA
/* 11864 */   MCD_OPC_FilterValue, 9, 14, 0, // Skip to: 11882
/* 11868 */   MCD_OPC_CheckPredicate, 12, 62, 7, // Skip to: 13726
/* 11872 */   MCD_OPC_CheckField, 21, 5, 0, 56, 7, // Skip to: 13726
/* 11878 */   MCD_OPC_Decode, 22, 201, 1, // Opcode: ABSQ_S_PH
/* 11882 */   MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 11895
/* 11886 */   MCD_OPC_CheckPredicate, 12, 44, 7, // Skip to: 13726
/* 11890 */   MCD_OPC_Decode, 243, 10, 202, 1, // Opcode: REPL_PH
/* 11895 */   MCD_OPC_FilterValue, 11, 15, 0, // Skip to: 11914
/* 11899 */   MCD_OPC_CheckPredicate, 12, 31, 7, // Skip to: 13726
/* 11903 */   MCD_OPC_CheckField, 21, 5, 0, 25, 7, // Skip to: 13726
/* 11909 */   MCD_OPC_Decode, 241, 10, 203, 1, // Opcode: REPLV_PH
/* 11914 */   MCD_OPC_FilterValue, 12, 15, 0, // Skip to: 11933
/* 11918 */   MCD_OPC_CheckPredicate, 12, 12, 7, // Skip to: 13726
/* 11922 */   MCD_OPC_CheckField, 21, 5, 0, 6, 7, // Skip to: 13726
/* 11928 */   MCD_OPC_Decode, 168, 10, 204, 1, // Opcode: PRECEQ_W_PHL
/* 11933 */   MCD_OPC_FilterValue, 13, 15, 0, // Skip to: 11952
/* 11937 */   MCD_OPC_CheckPredicate, 12, 249, 6, // Skip to: 13726
/* 11941 */   MCD_OPC_CheckField, 21, 5, 0, 243, 6, // Skip to: 13726
/* 11947 */   MCD_OPC_Decode, 169, 10, 204, 1, // Opcode: PRECEQ_W_PHR
/* 11952 */   MCD_OPC_FilterValue, 17, 14, 0, // Skip to: 11970
/* 11956 */   MCD_OPC_CheckPredicate, 12, 230, 6, // Skip to: 13726
/* 11960 */   MCD_OPC_CheckField, 21, 5, 0, 224, 6, // Skip to: 13726
/* 11966 */   MCD_OPC_Decode, 24, 205, 1, // Opcode: ABSQ_S_W
/* 11970 */   MCD_OPC_FilterValue, 27, 15, 0, // Skip to: 11989
/* 11974 */   MCD_OPC_CheckPredicate, 12, 212, 6, // Skip to: 13726
/* 11978 */   MCD_OPC_CheckField, 21, 5, 0, 206, 6, // Skip to: 13726
/* 11984 */   MCD_OPC_Decode, 249, 1, 205, 1, // Opcode: BITREV
/* 11989 */   MCD_OPC_FilterValue, 28, 15, 0, // Skip to: 12008
/* 11993 */   MCD_OPC_CheckPredicate, 12, 193, 6, // Skip to: 13726
/* 11997 */   MCD_OPC_CheckField, 21, 5, 0, 187, 6, // Skip to: 13726
/* 12003 */   MCD_OPC_Decode, 170, 10, 201, 1, // Opcode: PRECEU_PH_QBL
/* 12008 */   MCD_OPC_FilterValue, 29, 15, 0, // Skip to: 12027
/* 12012 */   MCD_OPC_CheckPredicate, 12, 174, 6, // Skip to: 13726
/* 12016 */   MCD_OPC_CheckField, 21, 5, 0, 168, 6, // Skip to: 13726
/* 12022 */   MCD_OPC_Decode, 172, 10, 201, 1, // Opcode: PRECEU_PH_QBR
/* 12027 */   MCD_OPC_FilterValue, 30, 15, 0, // Skip to: 12046
/* 12031 */   MCD_OPC_CheckPredicate, 12, 155, 6, // Skip to: 13726
/* 12035 */   MCD_OPC_CheckField, 21, 5, 0, 149, 6, // Skip to: 13726
/* 12041 */   MCD_OPC_Decode, 171, 10, 201, 1, // Opcode: PRECEU_PH_QBLA
/* 12046 */   MCD_OPC_FilterValue, 31, 140, 6, // Skip to: 13726
/* 12050 */   MCD_OPC_CheckPredicate, 12, 136, 6, // Skip to: 13726
/* 12054 */   MCD_OPC_CheckField, 21, 5, 0, 130, 6, // Skip to: 13726
/* 12060 */   MCD_OPC_Decode, 173, 10, 201, 1, // Opcode: PRECEU_PH_QBRA
/* 12065 */   MCD_OPC_FilterValue, 19, 31, 1, // Skip to: 12356
/* 12069 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 12072 */   MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 12085
/* 12076 */   MCD_OPC_CheckPredicate, 12, 110, 6, // Skip to: 13726
/* 12080 */   MCD_OPC_Decode, 199, 11, 206, 1, // Opcode: SHLL_QB
/* 12085 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 12098
/* 12089 */   MCD_OPC_CheckPredicate, 12, 97, 6, // Skip to: 13726
/* 12093 */   MCD_OPC_Decode, 215, 11, 206, 1, // Opcode: SHRL_QB
/* 12098 */   MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 12111
/* 12102 */   MCD_OPC_CheckPredicate, 12, 84, 6, // Skip to: 13726
/* 12106 */   MCD_OPC_Decode, 195, 11, 207, 1, // Opcode: SHLLV_QB
/* 12111 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 12124
/* 12115 */   MCD_OPC_CheckPredicate, 12, 71, 6, // Skip to: 13726
/* 12119 */   MCD_OPC_Decode, 213, 11, 207, 1, // Opcode: SHRLV_QB
/* 12124 */   MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 12137
/* 12128 */   MCD_OPC_CheckPredicate, 30, 58, 6, // Skip to: 13726
/* 12132 */   MCD_OPC_Decode, 208, 11, 206, 1, // Opcode: SHRA_QB
/* 12137 */   MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 12150
/* 12141 */   MCD_OPC_CheckPredicate, 30, 45, 6, // Skip to: 13726
/* 12145 */   MCD_OPC_Decode, 210, 11, 206, 1, // Opcode: SHRA_R_QB
/* 12150 */   MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 12163
/* 12154 */   MCD_OPC_CheckPredicate, 30, 32, 6, // Skip to: 13726
/* 12158 */   MCD_OPC_Decode, 203, 11, 207, 1, // Opcode: SHRAV_QB
/* 12163 */   MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 12176
/* 12167 */   MCD_OPC_CheckPredicate, 30, 19, 6, // Skip to: 13726
/* 12171 */   MCD_OPC_Decode, 205, 11, 207, 1, // Opcode: SHRAV_R_QB
/* 12176 */   MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 12189
/* 12180 */   MCD_OPC_CheckPredicate, 12, 6, 6, // Skip to: 13726
/* 12184 */   MCD_OPC_Decode, 198, 11, 206, 1, // Opcode: SHLL_PH
/* 12189 */   MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 12202
/* 12193 */   MCD_OPC_CheckPredicate, 12, 249, 5, // Skip to: 13726
/* 12197 */   MCD_OPC_Decode, 207, 11, 206, 1, // Opcode: SHRA_PH
/* 12202 */   MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 12215
/* 12206 */   MCD_OPC_CheckPredicate, 12, 236, 5, // Skip to: 13726
/* 12210 */   MCD_OPC_Decode, 194, 11, 207, 1, // Opcode: SHLLV_PH
/* 12215 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 12228
/* 12219 */   MCD_OPC_CheckPredicate, 12, 223, 5, // Skip to: 13726
/* 12223 */   MCD_OPC_Decode, 202, 11, 207, 1, // Opcode: SHRAV_PH
/* 12228 */   MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 12241
/* 12232 */   MCD_OPC_CheckPredicate, 12, 210, 5, // Skip to: 13726
/* 12236 */   MCD_OPC_Decode, 200, 11, 206, 1, // Opcode: SHLL_S_PH
/* 12241 */   MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 12254
/* 12245 */   MCD_OPC_CheckPredicate, 12, 197, 5, // Skip to: 13726
/* 12249 */   MCD_OPC_Decode, 209, 11, 206, 1, // Opcode: SHRA_R_PH
/* 12254 */   MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 12267
/* 12258 */   MCD_OPC_CheckPredicate, 12, 184, 5, // Skip to: 13726
/* 12262 */   MCD_OPC_Decode, 196, 11, 207, 1, // Opcode: SHLLV_S_PH
/* 12267 */   MCD_OPC_FilterValue, 15, 9, 0, // Skip to: 12280
/* 12271 */   MCD_OPC_CheckPredicate, 12, 171, 5, // Skip to: 13726
/* 12275 */   MCD_OPC_Decode, 204, 11, 207, 1, // Opcode: SHRAV_R_PH
/* 12280 */   MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 12293
/* 12284 */   MCD_OPC_CheckPredicate, 12, 158, 5, // Skip to: 13726
/* 12288 */   MCD_OPC_Decode, 201, 11, 208, 1, // Opcode: SHLL_S_W
/* 12293 */   MCD_OPC_FilterValue, 21, 9, 0, // Skip to: 12306
/* 12297 */   MCD_OPC_CheckPredicate, 12, 145, 5, // Skip to: 13726
/* 12301 */   MCD_OPC_Decode, 211, 11, 208, 1, // Opcode: SHRA_R_W
/* 12306 */   MCD_OPC_FilterValue, 22, 8, 0, // Skip to: 12318
/* 12310 */   MCD_OPC_CheckPredicate, 12, 132, 5, // Skip to: 13726
/* 12314 */   MCD_OPC_Decode, 197, 11, 36, // Opcode: SHLLV_S_W
/* 12318 */   MCD_OPC_FilterValue, 23, 8, 0, // Skip to: 12330
/* 12322 */   MCD_OPC_CheckPredicate, 12, 120, 5, // Skip to: 13726
/* 12326 */   MCD_OPC_Decode, 206, 11, 36, // Opcode: SHRAV_R_W
/* 12330 */   MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 12343
/* 12334 */   MCD_OPC_CheckPredicate, 30, 108, 5, // Skip to: 13726
/* 12338 */   MCD_OPC_Decode, 214, 11, 206, 1, // Opcode: SHRL_PH
/* 12343 */   MCD_OPC_FilterValue, 27, 99, 5, // Skip to: 13726
/* 12347 */   MCD_OPC_CheckPredicate, 30, 95, 5, // Skip to: 13726
/* 12351 */   MCD_OPC_Decode, 212, 11, 207, 1, // Opcode: SHRLV_PH
/* 12356 */   MCD_OPC_FilterValue, 24, 199, 0, // Skip to: 12559
/* 12360 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 12363 */   MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 12375
/* 12367 */   MCD_OPC_CheckPredicate, 30, 75, 5, // Skip to: 13726
/* 12371 */   MCD_OPC_Decode, 53, 195, 1, // Opcode: ADDUH_QB
/* 12375 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 12388
/* 12379 */   MCD_OPC_CheckPredicate, 30, 63, 5, // Skip to: 13726
/* 12383 */   MCD_OPC_Decode, 215, 12, 195, 1, // Opcode: SUBUH_QB
/* 12388 */   MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 12400
/* 12392 */   MCD_OPC_CheckPredicate, 30, 50, 5, // Skip to: 13726
/* 12396 */   MCD_OPC_Decode, 54, 195, 1, // Opcode: ADDUH_R_QB
/* 12400 */   MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 12413
/* 12404 */   MCD_OPC_CheckPredicate, 30, 38, 5, // Skip to: 13726
/* 12408 */   MCD_OPC_Decode, 216, 12, 195, 1, // Opcode: SUBUH_R_QB
/* 12413 */   MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 12425
/* 12417 */   MCD_OPC_CheckPredicate, 30, 25, 5, // Skip to: 13726
/* 12421 */   MCD_OPC_Decode, 32, 195, 1, // Opcode: ADDQH_PH
/* 12425 */   MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 12438
/* 12429 */   MCD_OPC_CheckPredicate, 30, 13, 5, // Skip to: 13726
/* 12433 */   MCD_OPC_Decode, 191, 12, 195, 1, // Opcode: SUBQH_PH
/* 12438 */   MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 12450
/* 12442 */   MCD_OPC_CheckPredicate, 30, 0, 5, // Skip to: 13726
/* 12446 */   MCD_OPC_Decode, 33, 195, 1, // Opcode: ADDQH_R_PH
/* 12450 */   MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 12463
/* 12454 */   MCD_OPC_CheckPredicate, 30, 244, 4, // Skip to: 13726
/* 12458 */   MCD_OPC_Decode, 192, 12, 195, 1, // Opcode: SUBQH_R_PH
/* 12463 */   MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 12476
/* 12467 */   MCD_OPC_CheckPredicate, 30, 231, 4, // Skip to: 13726
/* 12471 */   MCD_OPC_Decode, 218, 9, 195, 1, // Opcode: MUL_PH
/* 12476 */   MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 12489
/* 12480 */   MCD_OPC_CheckPredicate, 30, 218, 4, // Skip to: 13726
/* 12484 */   MCD_OPC_Decode, 222, 9, 195, 1, // Opcode: MUL_S_PH
/* 12489 */   MCD_OPC_FilterValue, 16, 7, 0, // Skip to: 12500
/* 12493 */   MCD_OPC_CheckPredicate, 30, 205, 4, // Skip to: 13726
/* 12497 */   MCD_OPC_Decode, 35, 35, // Opcode: ADDQH_W
/* 12500 */   MCD_OPC_FilterValue, 17, 8, 0, // Skip to: 12512
/* 12504 */   MCD_OPC_CheckPredicate, 30, 194, 4, // Skip to: 13726
/* 12508 */   MCD_OPC_Decode, 194, 12, 35, // Opcode: SUBQH_W
/* 12512 */   MCD_OPC_FilterValue, 18, 7, 0, // Skip to: 12523
/* 12516 */   MCD_OPC_CheckPredicate, 30, 182, 4, // Skip to: 13726
/* 12520 */   MCD_OPC_Decode, 34, 35, // Opcode: ADDQH_R_W
/* 12523 */   MCD_OPC_FilterValue, 19, 8, 0, // Skip to: 12535
/* 12527 */   MCD_OPC_CheckPredicate, 30, 171, 4, // Skip to: 13726
/* 12531 */   MCD_OPC_Decode, 193, 12, 35, // Opcode: SUBQH_R_W
/* 12535 */   MCD_OPC_FilterValue, 22, 8, 0, // Skip to: 12547
/* 12539 */   MCD_OPC_CheckPredicate, 30, 159, 4, // Skip to: 13726
/* 12543 */   MCD_OPC_Decode, 201, 9, 35, // Opcode: MULQ_S_W
/* 12547 */   MCD_OPC_FilterValue, 23, 151, 4, // Skip to: 13726
/* 12551 */   MCD_OPC_CheckPredicate, 30, 147, 4, // Skip to: 13726
/* 12555 */   MCD_OPC_Decode, 199, 9, 35, // Opcode: MULQ_RS_W
/* 12559 */   MCD_OPC_FilterValue, 32, 60, 0, // Skip to: 12623
/* 12563 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 12566 */   MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 12585
/* 12570 */   MCD_OPC_CheckPredicate, 6, 128, 4, // Skip to: 13726
/* 12574 */   MCD_OPC_CheckField, 21, 5, 0, 122, 4, // Skip to: 13726
/* 12580 */   MCD_OPC_Decode, 234, 13, 205, 1, // Opcode: WSBH
/* 12585 */   MCD_OPC_FilterValue, 16, 15, 0, // Skip to: 12604
/* 12589 */   MCD_OPC_CheckPredicate, 6, 109, 4, // Skip to: 13726
/* 12593 */   MCD_OPC_CheckField, 21, 5, 0, 103, 4, // Skip to: 13726
/* 12599 */   MCD_OPC_Decode, 168, 11, 205, 1, // Opcode: SEB
/* 12604 */   MCD_OPC_FilterValue, 24, 94, 4, // Skip to: 13726
/* 12608 */   MCD_OPC_CheckPredicate, 6, 90, 4, // Skip to: 13726
/* 12612 */   MCD_OPC_CheckField, 21, 5, 0, 84, 4, // Skip to: 13726
/* 12618 */   MCD_OPC_Decode, 171, 11, 205, 1, // Opcode: SEH
/* 12623 */   MCD_OPC_FilterValue, 48, 143, 1, // Skip to: 13026
/* 12627 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 12630 */   MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 12648
/* 12634 */   MCD_OPC_CheckPredicate, 30, 64, 4, // Skip to: 13726
/* 12638 */   MCD_OPC_CheckField, 13, 3, 0, 58, 4, // Skip to: 13726
/* 12644 */   MCD_OPC_Decode, 230, 4, 116, // Opcode: DPA_W_PH
/* 12648 */   MCD_OPC_FilterValue, 1, 14, 0, // Skip to: 12666
/* 12652 */   MCD_OPC_CheckPredicate, 30, 46, 4, // Skip to: 13726
/* 12656 */   MCD_OPC_CheckField, 13, 3, 0, 40, 4, // Skip to: 13726
/* 12662 */   MCD_OPC_Decode, 245, 4, 116, // Opcode: DPS_W_PH
/* 12666 */   MCD_OPC_FilterValue, 2, 14, 0, // Skip to: 12684
/* 12670 */   MCD_OPC_CheckPredicate, 30, 28, 4, // Skip to: 13726
/* 12674 */   MCD_OPC_CheckField, 13, 3, 0, 22, 4, // Skip to: 13726
/* 12680 */   MCD_OPC_Decode, 205, 9, 116, // Opcode: MULSA_W_PH
/* 12684 */   MCD_OPC_FilterValue, 3, 14, 0, // Skip to: 12702
/* 12688 */   MCD_OPC_CheckPredicate, 12, 10, 4, // Skip to: 13726
/* 12692 */   MCD_OPC_CheckField, 13, 3, 0, 4, 4, // Skip to: 13726
/* 12698 */   MCD_OPC_Decode, 227, 4, 116, // Opcode: DPAU_H_QBL
/* 12702 */   MCD_OPC_FilterValue, 4, 14, 0, // Skip to: 12720
/* 12706 */   MCD_OPC_CheckPredicate, 12, 248, 3, // Skip to: 13726
/* 12710 */   MCD_OPC_CheckField, 13, 3, 0, 242, 3, // Skip to: 13726
/* 12716 */   MCD_OPC_Decode, 226, 4, 116, // Opcode: DPAQ_S_W_PH
/* 12720 */   MCD_OPC_FilterValue, 5, 14, 0, // Skip to: 12738
/* 12724 */   MCD_OPC_CheckPredicate, 12, 230, 3, // Skip to: 13726
/* 12728 */   MCD_OPC_CheckField, 13, 3, 0, 224, 3, // Skip to: 13726
/* 12734 */   MCD_OPC_Decode, 235, 4, 116, // Opcode: DPSQ_S_W_PH
/* 12738 */   MCD_OPC_FilterValue, 6, 14, 0, // Skip to: 12756
/* 12742 */   MCD_OPC_CheckPredicate, 12, 212, 3, // Skip to: 13726
/* 12746 */   MCD_OPC_CheckField, 13, 3, 0, 206, 3, // Skip to: 13726
/* 12752 */   MCD_OPC_Decode, 204, 9, 116, // Opcode: MULSAQ_S_W_PH
/* 12756 */   MCD_OPC_FilterValue, 7, 14, 0, // Skip to: 12774
/* 12760 */   MCD_OPC_CheckPredicate, 12, 194, 3, // Skip to: 13726
/* 12764 */   MCD_OPC_CheckField, 13, 3, 0, 188, 3, // Skip to: 13726
/* 12770 */   MCD_OPC_Decode, 228, 4, 116, // Opcode: DPAU_H_QBR
/* 12774 */   MCD_OPC_FilterValue, 8, 14, 0, // Skip to: 12792
/* 12778 */   MCD_OPC_CheckPredicate, 30, 176, 3, // Skip to: 13726
/* 12782 */   MCD_OPC_CheckField, 13, 3, 0, 170, 3, // Skip to: 13726
/* 12788 */   MCD_OPC_Decode, 229, 4, 116, // Opcode: DPAX_W_PH
/* 12792 */   MCD_OPC_FilterValue, 9, 14, 0, // Skip to: 12810
/* 12796 */   MCD_OPC_CheckPredicate, 30, 158, 3, // Skip to: 13726
/* 12800 */   MCD_OPC_CheckField, 13, 3, 0, 152, 3, // Skip to: 13726
/* 12806 */   MCD_OPC_Decode, 244, 4, 116, // Opcode: DPSX_W_PH
/* 12810 */   MCD_OPC_FilterValue, 11, 14, 0, // Skip to: 12828
/* 12814 */   MCD_OPC_CheckPredicate, 12, 140, 3, // Skip to: 13726
/* 12818 */   MCD_OPC_CheckField, 13, 3, 0, 134, 3, // Skip to: 13726
/* 12824 */   MCD_OPC_Decode, 242, 4, 116, // Opcode: DPSU_H_QBL
/* 12828 */   MCD_OPC_FilterValue, 12, 14, 0, // Skip to: 12846
/* 12832 */   MCD_OPC_CheckPredicate, 12, 122, 3, // Skip to: 13726
/* 12836 */   MCD_OPC_CheckField, 13, 3, 0, 116, 3, // Skip to: 13726
/* 12842 */   MCD_OPC_Decode, 225, 4, 116, // Opcode: DPAQ_SA_L_W
/* 12846 */   MCD_OPC_FilterValue, 13, 14, 0, // Skip to: 12864
/* 12850 */   MCD_OPC_CheckPredicate, 12, 104, 3, // Skip to: 13726
/* 12854 */   MCD_OPC_CheckField, 13, 3, 0, 98, 3, // Skip to: 13726
/* 12860 */   MCD_OPC_Decode, 234, 4, 116, // Opcode: DPSQ_SA_L_W
/* 12864 */   MCD_OPC_FilterValue, 15, 14, 0, // Skip to: 12882
/* 12868 */   MCD_OPC_CheckPredicate, 12, 86, 3, // Skip to: 13726
/* 12872 */   MCD_OPC_CheckField, 13, 3, 0, 80, 3, // Skip to: 13726
/* 12878 */   MCD_OPC_Decode, 243, 4, 116, // Opcode: DPSU_H_QBR
/* 12882 */   MCD_OPC_FilterValue, 16, 14, 0, // Skip to: 12900
/* 12886 */   MCD_OPC_CheckPredicate, 12, 68, 3, // Skip to: 13726
/* 12890 */   MCD_OPC_CheckField, 13, 3, 0, 62, 3, // Skip to: 13726
/* 12896 */   MCD_OPC_Decode, 151, 8, 116, // Opcode: MAQ_SA_W_PHL
/* 12900 */   MCD_OPC_FilterValue, 18, 14, 0, // Skip to: 12918
/* 12904 */   MCD_OPC_CheckPredicate, 12, 50, 3, // Skip to: 13726
/* 12908 */   MCD_OPC_CheckField, 13, 3, 0, 44, 3, // Skip to: 13726
/* 12914 */   MCD_OPC_Decode, 152, 8, 116, // Opcode: MAQ_SA_W_PHR
/* 12918 */   MCD_OPC_FilterValue, 20, 14, 0, // Skip to: 12936
/* 12922 */   MCD_OPC_CheckPredicate, 12, 32, 3, // Skip to: 13726
/* 12926 */   MCD_OPC_CheckField, 13, 3, 0, 26, 3, // Skip to: 13726
/* 12932 */   MCD_OPC_Decode, 153, 8, 116, // Opcode: MAQ_S_W_PHL
/* 12936 */   MCD_OPC_FilterValue, 22, 14, 0, // Skip to: 12954
/* 12940 */   MCD_OPC_CheckPredicate, 12, 14, 3, // Skip to: 13726
/* 12944 */   MCD_OPC_CheckField, 13, 3, 0, 8, 3, // Skip to: 13726
/* 12950 */   MCD_OPC_Decode, 154, 8, 116, // Opcode: MAQ_S_W_PHR
/* 12954 */   MCD_OPC_FilterValue, 24, 14, 0, // Skip to: 12972
/* 12958 */   MCD_OPC_CheckPredicate, 30, 252, 2, // Skip to: 13726
/* 12962 */   MCD_OPC_CheckField, 13, 3, 0, 246, 2, // Skip to: 13726
/* 12968 */   MCD_OPC_Decode, 224, 4, 116, // Opcode: DPAQX_S_W_PH
/* 12972 */   MCD_OPC_FilterValue, 25, 14, 0, // Skip to: 12990
/* 12976 */   MCD_OPC_CheckPredicate, 30, 234, 2, // Skip to: 13726
/* 12980 */   MCD_OPC_CheckField, 13, 3, 0, 228, 2, // Skip to: 13726
/* 12986 */   MCD_OPC_Decode, 233, 4, 116, // Opcode: DPSQX_S_W_PH
/* 12990 */   MCD_OPC_FilterValue, 26, 14, 0, // Skip to: 13008
/* 12994 */   MCD_OPC_CheckPredicate, 30, 216, 2, // Skip to: 13726
/* 12998 */   MCD_OPC_CheckField, 13, 3, 0, 210, 2, // Skip to: 13726
/* 13004 */   MCD_OPC_Decode, 223, 4, 116, // Opcode: DPAQX_SA_W_PH
/* 13008 */   MCD_OPC_FilterValue, 27, 202, 2, // Skip to: 13726
/* 13012 */   MCD_OPC_CheckPredicate, 30, 198, 2, // Skip to: 13726
/* 13016 */   MCD_OPC_CheckField, 13, 3, 0, 192, 2, // Skip to: 13726
/* 13022 */   MCD_OPC_Decode, 232, 4, 116, // Opcode: DPSQX_SA_W_PH
/* 13026 */   MCD_OPC_FilterValue, 49, 41, 0, // Skip to: 13071
/* 13030 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 13033 */   MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 13045
/* 13037 */   MCD_OPC_CheckPredicate, 30, 173, 2, // Skip to: 13726
/* 13041 */   MCD_OPC_Decode, 96, 209, 1, // Opcode: APPEND
/* 13045 */   MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 13058
/* 13049 */   MCD_OPC_CheckPredicate, 30, 161, 2, // Skip to: 13726
/* 13053 */   MCD_OPC_Decode, 184, 10, 209, 1, // Opcode: PREPEND
/* 13058 */   MCD_OPC_FilterValue, 16, 152, 2, // Skip to: 13726
/* 13062 */   MCD_OPC_CheckPredicate, 30, 148, 2, // Skip to: 13726
/* 13066 */   MCD_OPC_Decode, 169, 1, 209, 1, // Opcode: BALIGN
/* 13071 */   MCD_OPC_FilterValue, 56, 58, 1, // Skip to: 13389
/* 13075 */   MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 13078 */   MCD_OPC_FilterValue, 0, 15, 0, // Skip to: 13097
/* 13082 */   MCD_OPC_CheckPredicate, 12, 128, 2, // Skip to: 13726
/* 13086 */   MCD_OPC_CheckField, 13, 3, 0, 122, 2, // Skip to: 13726
/* 13092 */   MCD_OPC_Decode, 157, 5, 210, 1, // Opcode: EXTR_W
/* 13097 */   MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 13116
/* 13101 */   MCD_OPC_CheckPredicate, 12, 109, 2, // Skip to: 13726
/* 13105 */   MCD_OPC_CheckField, 13, 3, 0, 103, 2, // Skip to: 13726
/* 13111 */   MCD_OPC_Decode, 153, 5, 211, 1, // Opcode: EXTRV_W
/* 13116 */   MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 13135
/* 13120 */   MCD_OPC_CheckPredicate, 12, 90, 2, // Skip to: 13726
/* 13124 */   MCD_OPC_CheckField, 13, 3, 0, 84, 2, // Skip to: 13726
/* 13130 */   MCD_OPC_Decode, 146, 5, 210, 1, // Opcode: EXTP
/* 13135 */   MCD_OPC_FilterValue, 3, 15, 0, // Skip to: 13154
/* 13139 */   MCD_OPC_CheckPredicate, 12, 71, 2, // Skip to: 13726
/* 13143 */   MCD_OPC_CheckField, 13, 3, 0, 65, 2, // Skip to: 13726
/* 13149 */   MCD_OPC_Decode, 149, 5, 211, 1, // Opcode: EXTPV
/* 13154 */   MCD_OPC_FilterValue, 4, 15, 0, // Skip to: 13173
/* 13158 */   MCD_OPC_CheckPredicate, 12, 52, 2, // Skip to: 13726
/* 13162 */   MCD_OPC_CheckField, 13, 3, 0, 46, 2, // Skip to: 13726
/* 13168 */   MCD_OPC_Decode, 155, 5, 210, 1, // Opcode: EXTR_R_W
/* 13173 */   MCD_OPC_FilterValue, 5, 15, 0, // Skip to: 13192
/* 13177 */   MCD_OPC_CheckPredicate, 12, 33, 2, // Skip to: 13726
/* 13181 */   MCD_OPC_CheckField, 13, 3, 0, 27, 2, // Skip to: 13726
/* 13187 */   MCD_OPC_Decode, 151, 5, 211, 1, // Opcode: EXTRV_R_W
/* 13192 */   MCD_OPC_FilterValue, 6, 15, 0, // Skip to: 13211
/* 13196 */   MCD_OPC_CheckPredicate, 12, 14, 2, // Skip to: 13726
/* 13200 */   MCD_OPC_CheckField, 13, 3, 0, 8, 2, // Skip to: 13726
/* 13206 */   MCD_OPC_Decode, 154, 5, 210, 1, // Opcode: EXTR_RS_W
/* 13211 */   MCD_OPC_FilterValue, 7, 15, 0, // Skip to: 13230
/* 13215 */   MCD_OPC_CheckPredicate, 12, 251, 1, // Skip to: 13726
/* 13219 */   MCD_OPC_CheckField, 13, 3, 0, 245, 1, // Skip to: 13726
/* 13225 */   MCD_OPC_Decode, 150, 5, 211, 1, // Opcode: EXTRV_RS_W
/* 13230 */   MCD_OPC_FilterValue, 10, 15, 0, // Skip to: 13249
/* 13234 */   MCD_OPC_CheckPredicate, 12, 232, 1, // Skip to: 13726
/* 13238 */   MCD_OPC_CheckField, 13, 3, 0, 226, 1, // Skip to: 13726
/* 13244 */   MCD_OPC_Decode, 147, 5, 210, 1, // Opcode: EXTPDP
/* 13249 */   MCD_OPC_FilterValue, 11, 15, 0, // Skip to: 13268
/* 13253 */   MCD_OPC_CheckPredicate, 12, 213, 1, // Skip to: 13726
/* 13257 */   MCD_OPC_CheckField, 13, 3, 0, 207, 1, // Skip to: 13726
/* 13263 */   MCD_OPC_Decode, 148, 5, 211, 1, // Opcode: EXTPDPV
/* 13268 */   MCD_OPC_FilterValue, 14, 15, 0, // Skip to: 13287
/* 13272 */   MCD_OPC_CheckPredicate, 12, 194, 1, // Skip to: 13726
/* 13276 */   MCD_OPC_CheckField, 13, 3, 0, 188, 1, // Skip to: 13726
/* 13282 */   MCD_OPC_Decode, 156, 5, 210, 1, // Opcode: EXTR_S_H
/* 13287 */   MCD_OPC_FilterValue, 15, 15, 0, // Skip to: 13306
/* 13291 */   MCD_OPC_CheckPredicate, 12, 175, 1, // Skip to: 13726
/* 13295 */   MCD_OPC_CheckField, 13, 3, 0, 169, 1, // Skip to: 13726
/* 13301 */   MCD_OPC_Decode, 152, 5, 211, 1, // Opcode: EXTRV_S_H
/* 13306 */   MCD_OPC_FilterValue, 18, 9, 0, // Skip to: 13319
/* 13310 */   MCD_OPC_CheckPredicate, 12, 156, 1, // Skip to: 13726
/* 13314 */   MCD_OPC_Decode, 237, 10, 212, 1, // Opcode: RDDSP
/* 13319 */   MCD_OPC_FilterValue, 19, 9, 0, // Skip to: 13332
/* 13323 */   MCD_OPC_CheckPredicate, 12, 143, 1, // Skip to: 13726
/* 13327 */   MCD_OPC_Decode, 233, 13, 213, 1, // Opcode: WRDSP
/* 13332 */   MCD_OPC_FilterValue, 26, 15, 0, // Skip to: 13351
/* 13336 */   MCD_OPC_CheckPredicate, 12, 130, 1, // Skip to: 13726
/* 13340 */   MCD_OPC_CheckField, 13, 7, 0, 124, 1, // Skip to: 13726
/* 13346 */   MCD_OPC_Decode, 192, 11, 214, 1, // Opcode: SHILO
/* 13351 */   MCD_OPC_FilterValue, 27, 15, 0, // Skip to: 13370
/* 13355 */   MCD_OPC_CheckPredicate, 12, 111, 1, // Skip to: 13726
/* 13359 */   MCD_OPC_CheckField, 13, 8, 0, 105, 1, // Skip to: 13726
/* 13365 */   MCD_OPC_Decode, 193, 11, 215, 1, // Opcode: SHILOV
/* 13370 */   MCD_OPC_FilterValue, 31, 96, 1, // Skip to: 13726
/* 13374 */   MCD_OPC_CheckPredicate, 12, 92, 1, // Skip to: 13726
/* 13378 */   MCD_OPC_CheckField, 13, 8, 0, 86, 1, // Skip to: 13726
/* 13384 */   MCD_OPC_Decode, 180, 9, 215, 1, // Opcode: MTHLIP
/* 13389 */   MCD_OPC_FilterValue, 59, 77, 1, // Skip to: 13726
/* 13393 */   MCD_OPC_CheckPredicate, 5, 73, 1, // Skip to: 13726
/* 13397 */   MCD_OPC_CheckField, 21, 5, 0, 67, 1, // Skip to: 13726
/* 13403 */   MCD_OPC_CheckField, 6, 5, 0, 61, 1, // Skip to: 13726
/* 13409 */   MCD_OPC_Decode, 238, 10, 216, 1, // Opcode: RDHWR
/* 13414 */   MCD_OPC_FilterValue, 32, 9, 0, // Skip to: 13427
/* 13418 */   MCD_OPC_CheckPredicate, 5, 48, 1, // Skip to: 13726
/* 13422 */   MCD_OPC_Decode, 153, 7, 217, 1, // Opcode: LB
/* 13427 */   MCD_OPC_FilterValue, 33, 9, 0, // Skip to: 13440
/* 13431 */   MCD_OPC_CheckPredicate, 5, 35, 1, // Skip to: 13726
/* 13435 */   MCD_OPC_Decode, 184, 7, 217, 1, // Opcode: LH
/* 13440 */   MCD_OPC_FilterValue, 34, 9, 0, // Skip to: 13453
/* 13444 */   MCD_OPC_CheckPredicate, 11, 22, 1, // Skip to: 13726
/* 13448 */   MCD_OPC_Decode, 222, 7, 217, 1, // Opcode: LWL
/* 13453 */   MCD_OPC_FilterValue, 35, 9, 0, // Skip to: 13466
/* 13457 */   MCD_OPC_CheckPredicate, 1, 9, 1, // Skip to: 13726
/* 13461 */   MCD_OPC_Decode, 213, 7, 217, 1, // Opcode: LW
/* 13466 */   MCD_OPC_FilterValue, 36, 9, 0, // Skip to: 13479
/* 13470 */   MCD_OPC_CheckPredicate, 5, 252, 0, // Skip to: 13726
/* 13474 */   MCD_OPC_Decode, 158, 7, 217, 1, // Opcode: LBu
/* 13479 */   MCD_OPC_FilterValue, 37, 9, 0, // Skip to: 13492
/* 13483 */   MCD_OPC_CheckPredicate, 5, 239, 0, // Skip to: 13726
/* 13487 */   MCD_OPC_Decode, 189, 7, 217, 1, // Opcode: LHu
/* 13492 */   MCD_OPC_FilterValue, 38, 9, 0, // Skip to: 13505
/* 13496 */   MCD_OPC_CheckPredicate, 11, 226, 0, // Skip to: 13726
/* 13500 */   MCD_OPC_Decode, 230, 7, 217, 1, // Opcode: LWR
/* 13505 */   MCD_OPC_FilterValue, 40, 9, 0, // Skip to: 13518
/* 13509 */   MCD_OPC_CheckPredicate, 5, 213, 0, // Skip to: 13726
/* 13513 */   MCD_OPC_Decode, 142, 11, 217, 1, // Opcode: SB
/* 13518 */   MCD_OPC_FilterValue, 41, 9, 0, // Skip to: 13531
/* 13522 */   MCD_OPC_CheckPredicate, 5, 200, 0, // Skip to: 13726
/* 13526 */   MCD_OPC_Decode, 186, 11, 217, 1, // Opcode: SH
/* 13531 */   MCD_OPC_FilterValue, 42, 9, 0, // Skip to: 13544
/* 13535 */   MCD_OPC_CheckPredicate, 11, 187, 0, // Skip to: 13726
/* 13539 */   MCD_OPC_Decode, 243, 12, 217, 1, // Opcode: SWL
/* 13544 */   MCD_OPC_FilterValue, 43, 9, 0, // Skip to: 13557
/* 13548 */   MCD_OPC_CheckPredicate, 1, 174, 0, // Skip to: 13726
/* 13552 */   MCD_OPC_Decode, 235, 12, 217, 1, // Opcode: SW
/* 13557 */   MCD_OPC_FilterValue, 46, 9, 0, // Skip to: 13570
/* 13561 */   MCD_OPC_CheckPredicate, 11, 161, 0, // Skip to: 13726
/* 13565 */   MCD_OPC_Decode, 250, 12, 217, 1, // Opcode: SWR
/* 13570 */   MCD_OPC_FilterValue, 47, 9, 0, // Skip to: 13583
/* 13574 */   MCD_OPC_CheckPredicate, 31, 148, 0, // Skip to: 13726
/* 13578 */   MCD_OPC_Decode, 220, 2, 218, 1, // Opcode: CACHE
/* 13583 */   MCD_OPC_FilterValue, 48, 9, 0, // Skip to: 13596
/* 13587 */   MCD_OPC_CheckPredicate, 32, 135, 0, // Skip to: 13726
/* 13591 */   MCD_OPC_Decode, 193, 7, 217, 1, // Opcode: LL
/* 13596 */   MCD_OPC_FilterValue, 49, 9, 0, // Skip to: 13609
/* 13600 */   MCD_OPC_CheckPredicate, 5, 122, 0, // Skip to: 13726
/* 13604 */   MCD_OPC_Decode, 216, 7, 219, 1, // Opcode: LWC1
/* 13609 */   MCD_OPC_FilterValue, 50, 9, 0, // Skip to: 13622
/* 13613 */   MCD_OPC_CheckPredicate, 33, 109, 0, // Skip to: 13726
/* 13617 */   MCD_OPC_Decode, 218, 7, 220, 1, // Opcode: LWC2
/* 13622 */   MCD_OPC_FilterValue, 51, 9, 0, // Skip to: 13635
/* 13626 */   MCD_OPC_CheckPredicate, 31, 96, 0, // Skip to: 13726
/* 13630 */   MCD_OPC_Decode, 181, 10, 218, 1, // Opcode: PREF
/* 13635 */   MCD_OPC_FilterValue, 53, 9, 0, // Skip to: 13648
/* 13639 */   MCD_OPC_CheckPredicate, 34, 83, 0, // Skip to: 13726
/* 13643 */   MCD_OPC_Decode, 162, 7, 219, 1, // Opcode: LDC1
/* 13648 */   MCD_OPC_FilterValue, 54, 9, 0, // Skip to: 13661
/* 13652 */   MCD_OPC_CheckPredicate, 35, 70, 0, // Skip to: 13726
/* 13656 */   MCD_OPC_Decode, 165, 7, 220, 1, // Opcode: LDC2
/* 13661 */   MCD_OPC_FilterValue, 56, 9, 0, // Skip to: 13674
/* 13665 */   MCD_OPC_CheckPredicate, 32, 57, 0, // Skip to: 13726
/* 13669 */   MCD_OPC_Decode, 146, 11, 217, 1, // Opcode: SC
/* 13674 */   MCD_OPC_FilterValue, 57, 9, 0, // Skip to: 13687
/* 13678 */   MCD_OPC_CheckPredicate, 5, 44, 0, // Skip to: 13726
/* 13682 */   MCD_OPC_Decode, 238, 12, 219, 1, // Opcode: SWC1
/* 13687 */   MCD_OPC_FilterValue, 58, 9, 0, // Skip to: 13700
/* 13691 */   MCD_OPC_CheckPredicate, 33, 31, 0, // Skip to: 13726
/* 13695 */   MCD_OPC_Decode, 240, 12, 220, 1, // Opcode: SWC2
/* 13700 */   MCD_OPC_FilterValue, 61, 9, 0, // Skip to: 13713
/* 13704 */   MCD_OPC_CheckPredicate, 34, 18, 0, // Skip to: 13726
/* 13708 */   MCD_OPC_Decode, 156, 11, 219, 1, // Opcode: SDC1
/* 13713 */   MCD_OPC_FilterValue, 62, 9, 0, // Skip to: 13726
/* 13717 */   MCD_OPC_CheckPredicate, 35, 5, 0, // Skip to: 13726
/* 13721 */   MCD_OPC_Decode, 159, 11, 220, 1, // Opcode: SDC2
/* 13726 */   MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTableMips32r6_64r632[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 0, 205, 1, // Skip to: 468
/* 7 */       MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 10 */      MCD_OPC_FilterValue, 5, 15, 0, // Skip to: 29
/* 14 */      MCD_OPC_CheckPredicate, 36, 37, 7, // Skip to: 1847
/* 18 */      MCD_OPC_CheckField, 8, 3, 0, 31, 7, // Skip to: 1847
/* 24 */      MCD_OPC_Decode, 206, 7, 221, 1, // Opcode: LSA_R6
/* 29 */      MCD_OPC_FilterValue, 9, 14, 0, // Skip to: 47
/* 33 */      MCD_OPC_CheckPredicate, 36, 18, 7, // Skip to: 1847
/* 37 */      MCD_OPC_CheckField, 6, 15, 16, 12, 7, // Skip to: 1847
/* 43 */      MCD_OPC_Decode, 142, 7, 61, // Opcode: JR_HB_R6
/* 47 */      MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 59
/* 51 */      MCD_OPC_CheckPredicate, 36, 0, 7, // Skip to: 1847
/* 55 */      MCD_OPC_Decode, 155, 11, 64, // Opcode: SDBBP_R6
/* 59 */      MCD_OPC_FilterValue, 16, 20, 0, // Skip to: 83
/* 63 */      MCD_OPC_CheckPredicate, 36, 244, 6, // Skip to: 1847
/* 67 */      MCD_OPC_CheckField, 16, 5, 0, 238, 6, // Skip to: 1847
/* 73 */      MCD_OPC_CheckField, 6, 5, 1, 232, 6, // Skip to: 1847
/* 79 */      MCD_OPC_Decode, 154, 3, 62, // Opcode: CLZ_R6
/* 83 */      MCD_OPC_FilterValue, 17, 20, 0, // Skip to: 107
/* 87 */      MCD_OPC_CheckPredicate, 36, 220, 6, // Skip to: 1847
/* 91 */      MCD_OPC_CheckField, 16, 5, 0, 214, 6, // Skip to: 1847
/* 97 */      MCD_OPC_CheckField, 6, 5, 1, 208, 6, // Skip to: 1847
/* 103 */     MCD_OPC_Decode, 135, 3, 62, // Opcode: CLO_R6
/* 107 */     MCD_OPC_FilterValue, 18, 21, 0, // Skip to: 132
/* 111 */     MCD_OPC_CheckPredicate, 37, 196, 6, // Skip to: 1847
/* 115 */     MCD_OPC_CheckField, 16, 5, 0, 190, 6, // Skip to: 1847
/* 121 */     MCD_OPC_CheckField, 6, 5, 1, 184, 6, // Skip to: 1847
/* 127 */     MCD_OPC_Decode, 171, 4, 222, 1, // Opcode: DCLZ_R6
/* 132 */     MCD_OPC_FilterValue, 19, 21, 0, // Skip to: 157
/* 136 */     MCD_OPC_CheckPredicate, 37, 171, 6, // Skip to: 1847
/* 140 */     MCD_OPC_CheckField, 16, 5, 0, 165, 6, // Skip to: 1847
/* 146 */     MCD_OPC_CheckField, 6, 5, 1, 159, 6, // Skip to: 1847
/* 152 */     MCD_OPC_Decode, 169, 4, 222, 1, // Opcode: DCLO_R6
/* 157 */     MCD_OPC_FilterValue, 21, 15, 0, // Skip to: 176
/* 161 */     MCD_OPC_CheckPredicate, 37, 146, 6, // Skip to: 1847
/* 165 */     MCD_OPC_CheckField, 8, 3, 0, 140, 6, // Skip to: 1847
/* 171 */     MCD_OPC_Decode, 195, 4, 223, 1, // Opcode: DLSA_R6
/* 176 */     MCD_OPC_FilterValue, 24, 27, 0, // Skip to: 207
/* 180 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 183 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 195
/* 187 */     MCD_OPC_CheckPredicate, 36, 120, 6, // Skip to: 1847
/* 191 */     MCD_OPC_Decode, 221, 9, 35, // Opcode: MUL_R6
/* 195 */     MCD_OPC_FilterValue, 3, 112, 6, // Skip to: 1847
/* 199 */     MCD_OPC_CheckPredicate, 36, 108, 6, // Skip to: 1847
/* 203 */     MCD_OPC_Decode, 191, 9, 35, // Opcode: MUH
/* 207 */     MCD_OPC_FilterValue, 25, 27, 0, // Skip to: 238
/* 211 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 214 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 226
/* 218 */     MCD_OPC_CheckPredicate, 36, 89, 6, // Skip to: 1847
/* 222 */     MCD_OPC_Decode, 212, 9, 35, // Opcode: MULU
/* 226 */     MCD_OPC_FilterValue, 3, 81, 6, // Skip to: 1847
/* 230 */     MCD_OPC_CheckPredicate, 36, 77, 6, // Skip to: 1847
/* 234 */     MCD_OPC_Decode, 192, 9, 35, // Opcode: MUHU
/* 238 */     MCD_OPC_FilterValue, 26, 27, 0, // Skip to: 269
/* 242 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 245 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 257
/* 249 */     MCD_OPC_CheckPredicate, 36, 58, 6, // Skip to: 1847
/* 253 */     MCD_OPC_Decode, 183, 4, 35, // Opcode: DIV
/* 257 */     MCD_OPC_FilterValue, 3, 50, 6, // Skip to: 1847
/* 261 */     MCD_OPC_CheckPredicate, 36, 46, 6, // Skip to: 1847
/* 265 */     MCD_OPC_Decode, 222, 8, 35, // Opcode: MOD
/* 269 */     MCD_OPC_FilterValue, 27, 27, 0, // Skip to: 300
/* 273 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 276 */     MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 288
/* 280 */     MCD_OPC_CheckPredicate, 36, 27, 6, // Skip to: 1847
/* 284 */     MCD_OPC_Decode, 184, 4, 35, // Opcode: DIVU
/* 288 */     MCD_OPC_FilterValue, 3, 19, 6, // Skip to: 1847
/* 292 */     MCD_OPC_CheckPredicate, 36, 15, 6, // Skip to: 1847
/* 296 */     MCD_OPC_Decode, 224, 8, 35, // Opcode: MODU
/* 300 */     MCD_OPC_FilterValue, 28, 29, 0, // Skip to: 333
/* 304 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 307 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 320
/* 311 */     MCD_OPC_CheckPredicate, 37, 252, 5, // Skip to: 1847
/* 315 */     MCD_OPC_Decode, 210, 4, 224, 1, // Opcode: DMUL_R6
/* 320 */     MCD_OPC_FilterValue, 3, 243, 5, // Skip to: 1847
/* 324 */     MCD_OPC_CheckPredicate, 37, 239, 5, // Skip to: 1847
/* 328 */     MCD_OPC_Decode, 204, 4, 224, 1, // Opcode: DMUH
/* 333 */     MCD_OPC_FilterValue, 29, 29, 0, // Skip to: 366
/* 337 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 340 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 353
/* 344 */     MCD_OPC_CheckPredicate, 37, 219, 5, // Skip to: 1847
/* 348 */     MCD_OPC_Decode, 209, 4, 224, 1, // Opcode: DMULU
/* 353 */     MCD_OPC_FilterValue, 3, 210, 5, // Skip to: 1847
/* 357 */     MCD_OPC_CheckPredicate, 37, 206, 5, // Skip to: 1847
/* 361 */     MCD_OPC_Decode, 205, 4, 224, 1, // Opcode: DMUHU
/* 366 */     MCD_OPC_FilterValue, 30, 29, 0, // Skip to: 399
/* 370 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 373 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 386
/* 377 */     MCD_OPC_CheckPredicate, 37, 186, 5, // Skip to: 1847
/* 381 */     MCD_OPC_Decode, 172, 4, 224, 1, // Opcode: DDIV
/* 386 */     MCD_OPC_FilterValue, 3, 177, 5, // Skip to: 1847
/* 390 */     MCD_OPC_CheckPredicate, 37, 173, 5, // Skip to: 1847
/* 394 */     MCD_OPC_Decode, 199, 4, 224, 1, // Opcode: DMOD
/* 399 */     MCD_OPC_FilterValue, 31, 29, 0, // Skip to: 432
/* 403 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 406 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 419
/* 410 */     MCD_OPC_CheckPredicate, 37, 153, 5, // Skip to: 1847
/* 414 */     MCD_OPC_Decode, 173, 4, 224, 1, // Opcode: DDIVU
/* 419 */     MCD_OPC_FilterValue, 3, 144, 5, // Skip to: 1847
/* 423 */     MCD_OPC_CheckPredicate, 37, 140, 5, // Skip to: 1847
/* 427 */     MCD_OPC_Decode, 200, 4, 224, 1, // Opcode: DMODU
/* 432 */     MCD_OPC_FilterValue, 53, 14, 0, // Skip to: 450
/* 436 */     MCD_OPC_CheckPredicate, 38, 127, 5, // Skip to: 1847
/* 440 */     MCD_OPC_CheckField, 6, 5, 0, 121, 5, // Skip to: 1847
/* 446 */     MCD_OPC_Decode, 174, 11, 35, // Opcode: SELEQZ
/* 450 */     MCD_OPC_FilterValue, 55, 113, 5, // Skip to: 1847
/* 454 */     MCD_OPC_CheckPredicate, 38, 109, 5, // Skip to: 1847
/* 458 */     MCD_OPC_CheckField, 6, 5, 0, 103, 5, // Skip to: 1847
/* 464 */     MCD_OPC_Decode, 178, 11, 35, // Opcode: SELNEZ
/* 468 */     MCD_OPC_FilterValue, 1, 47, 0, // Skip to: 519
/* 472 */     MCD_OPC_ExtractField, 16, 5,  // Inst{20-16} ...
/* 475 */     MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 488
/* 479 */     MCD_OPC_CheckPredicate, 37, 84, 5, // Skip to: 1847
/* 483 */     MCD_OPC_Decode, 163, 4, 225, 1, // Opcode: DAHI
/* 488 */     MCD_OPC_FilterValue, 17, 14, 0, // Skip to: 506
/* 492 */     MCD_OPC_CheckPredicate, 36, 71, 5, // Skip to: 1847
/* 496 */     MCD_OPC_CheckField, 21, 5, 0, 65, 5, // Skip to: 1847
/* 502 */     MCD_OPC_Decode, 167, 1, 75, // Opcode: BAL
/* 506 */     MCD_OPC_FilterValue, 30, 57, 5, // Skip to: 1847
/* 510 */     MCD_OPC_CheckPredicate, 37, 53, 5, // Skip to: 1847
/* 514 */     MCD_OPC_Decode, 165, 4, 225, 1, // Opcode: DATI
/* 519 */     MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 532
/* 523 */     MCD_OPC_CheckPredicate, 36, 40, 5, // Skip to: 1847
/* 527 */     MCD_OPC_Decode, 220, 1, 226, 1, // Opcode: BGEZALC
/* 532 */     MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 545
/* 536 */     MCD_OPC_CheckPredicate, 36, 27, 5, // Skip to: 1847
/* 540 */     MCD_OPC_Decode, 134, 2, 227, 1, // Opcode: BLTZALC
/* 545 */     MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 558
/* 549 */     MCD_OPC_CheckPredicate, 36, 14, 5, // Skip to: 1847
/* 553 */     MCD_OPC_Decode, 208, 1, 228, 1, // Opcode: BEQC
/* 558 */     MCD_OPC_FilterValue, 15, 8, 0, // Skip to: 570
/* 562 */     MCD_OPC_CheckPredicate, 36, 1, 5, // Skip to: 1847
/* 566 */     MCD_OPC_Decode, 137, 1, 47, // Opcode: AUI
/* 570 */     MCD_OPC_FilterValue, 17, 5, 3, // Skip to: 1347
/* 574 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 577 */     MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 590
/* 581 */     MCD_OPC_CheckPredicate, 36, 238, 4, // Skip to: 1847
/* 585 */     MCD_OPC_Decode, 180, 1, 229, 1, // Opcode: BC1EQZ
/* 590 */     MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 603
/* 594 */     MCD_OPC_CheckPredicate, 36, 225, 4, // Skip to: 1847
/* 598 */     MCD_OPC_Decode, 184, 1, 229, 1, // Opcode: BC1NEZ
/* 603 */     MCD_OPC_FilterValue, 16, 150, 0, // Skip to: 757
/* 607 */     MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 610 */     MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 623
/* 614 */     MCD_OPC_CheckPredicate, 36, 205, 4, // Skip to: 1847
/* 618 */     MCD_OPC_Decode, 183, 11, 230, 1, // Opcode: SEL_S
/* 623 */     MCD_OPC_FilterValue, 20, 8, 0, // Skip to: 635
/* 627 */     MCD_OPC_CheckPredicate, 36, 192, 4, // Skip to: 1847
/* 631 */     MCD_OPC_Decode, 177, 11, 93, // Opcode: SELEQZ_S
/* 635 */     MCD_OPC_FilterValue, 23, 8, 0, // Skip to: 647
/* 639 */     MCD_OPC_CheckPredicate, 36, 180, 4, // Skip to: 1847
/* 643 */     MCD_OPC_Decode, 181, 11, 93, // Opcode: SELNEZ_S
/* 647 */     MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 660
/* 651 */     MCD_OPC_CheckPredicate, 36, 168, 4, // Skip to: 1847
/* 655 */     MCD_OPC_Decode, 132, 8, 231, 1, // Opcode: MADDF_S
/* 660 */     MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 673
/* 664 */     MCD_OPC_CheckPredicate, 36, 155, 4, // Skip to: 1847
/* 668 */     MCD_OPC_Decode, 150, 9, 231, 1, // Opcode: MSUBF_S
/* 673 */     MCD_OPC_FilterValue, 26, 14, 0, // Skip to: 691
/* 677 */     MCD_OPC_CheckPredicate, 36, 142, 4, // Skip to: 1847
/* 681 */     MCD_OPC_CheckField, 16, 5, 0, 136, 4, // Skip to: 1847
/* 687 */     MCD_OPC_Decode, 246, 10, 94, // Opcode: RINT_S
/* 691 */     MCD_OPC_FilterValue, 27, 14, 0, // Skip to: 709
/* 695 */     MCD_OPC_CheckPredicate, 36, 124, 4, // Skip to: 1847
/* 699 */     MCD_OPC_CheckField, 16, 5, 0, 118, 4, // Skip to: 1847
/* 705 */     MCD_OPC_Decode, 244, 2, 94, // Opcode: CLASS_S
/* 709 */     MCD_OPC_FilterValue, 28, 8, 0, // Skip to: 721
/* 713 */     MCD_OPC_CheckPredicate, 36, 106, 4, // Skip to: 1847
/* 717 */     MCD_OPC_Decode, 211, 8, 93, // Opcode: MIN_S
/* 721 */     MCD_OPC_FilterValue, 29, 8, 0, // Skip to: 733
/* 725 */     MCD_OPC_CheckPredicate, 36, 94, 4, // Skip to: 1847
/* 729 */     MCD_OPC_Decode, 170, 8, 93, // Opcode: MAX_S
/* 733 */     MCD_OPC_FilterValue, 30, 8, 0, // Skip to: 745
/* 737 */     MCD_OPC_CheckPredicate, 36, 82, 4, // Skip to: 1847
/* 741 */     MCD_OPC_Decode, 197, 8, 93, // Opcode: MINA_S
/* 745 */     MCD_OPC_FilterValue, 31, 74, 4, // Skip to: 1847
/* 749 */     MCD_OPC_CheckPredicate, 36, 70, 4, // Skip to: 1847
/* 753 */     MCD_OPC_Decode, 156, 8, 93, // Opcode: MAXA_S
/* 757 */     MCD_OPC_FilterValue, 17, 156, 0, // Skip to: 917
/* 761 */     MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 764 */     MCD_OPC_FilterValue, 16, 9, 0, // Skip to: 777
/* 768 */     MCD_OPC_CheckPredicate, 36, 51, 4, // Skip to: 1847
/* 772 */     MCD_OPC_Decode, 182, 11, 232, 1, // Opcode: SEL_D
/* 777 */     MCD_OPC_FilterValue, 20, 9, 0, // Skip to: 790
/* 781 */     MCD_OPC_CheckPredicate, 36, 38, 4, // Skip to: 1847
/* 785 */     MCD_OPC_Decode, 176, 11, 233, 1, // Opcode: SELEQZ_D
/* 790 */     MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 803
/* 794 */     MCD_OPC_CheckPredicate, 36, 25, 4, // Skip to: 1847
/* 798 */     MCD_OPC_Decode, 180, 11, 233, 1, // Opcode: SELNEZ_D
/* 803 */     MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 816
/* 807 */     MCD_OPC_CheckPredicate, 36, 12, 4, // Skip to: 1847
/* 811 */     MCD_OPC_Decode, 131, 8, 234, 1, // Opcode: MADDF_D
/* 816 */     MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 829
/* 820 */     MCD_OPC_CheckPredicate, 36, 255, 3, // Skip to: 1847
/* 824 */     MCD_OPC_Decode, 149, 9, 234, 1, // Opcode: MSUBF_D
/* 829 */     MCD_OPC_FilterValue, 26, 14, 0, // Skip to: 847
/* 833 */     MCD_OPC_CheckPredicate, 36, 242, 3, // Skip to: 1847
/* 837 */     MCD_OPC_CheckField, 16, 5, 0, 236, 3, // Skip to: 1847
/* 843 */     MCD_OPC_Decode, 245, 10, 105, // Opcode: RINT_D
/* 847 */     MCD_OPC_FilterValue, 27, 14, 0, // Skip to: 865
/* 851 */     MCD_OPC_CheckPredicate, 36, 224, 3, // Skip to: 1847
/* 855 */     MCD_OPC_CheckField, 16, 5, 0, 218, 3, // Skip to: 1847
/* 861 */     MCD_OPC_Decode, 243, 2, 105, // Opcode: CLASS_D
/* 865 */     MCD_OPC_FilterValue, 28, 9, 0, // Skip to: 878
/* 869 */     MCD_OPC_CheckPredicate, 36, 206, 3, // Skip to: 1847
/* 873 */     MCD_OPC_Decode, 210, 8, 233, 1, // Opcode: MIN_D
/* 878 */     MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 891
/* 882 */     MCD_OPC_CheckPredicate, 36, 193, 3, // Skip to: 1847
/* 886 */     MCD_OPC_Decode, 169, 8, 233, 1, // Opcode: MAX_D
/* 891 */     MCD_OPC_FilterValue, 30, 9, 0, // Skip to: 904
/* 895 */     MCD_OPC_CheckPredicate, 36, 180, 3, // Skip to: 1847
/* 899 */     MCD_OPC_Decode, 196, 8, 233, 1, // Opcode: MINA_D
/* 904 */     MCD_OPC_FilterValue, 31, 171, 3, // Skip to: 1847
/* 908 */     MCD_OPC_CheckPredicate, 36, 167, 3, // Skip to: 1847
/* 912 */     MCD_OPC_Decode, 155, 8, 233, 1, // Opcode: MAXA_D
/* 917 */     MCD_OPC_FilterValue, 20, 211, 0, // Skip to: 1132
/* 921 */     MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 924 */     MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 937
/* 928 */     MCD_OPC_CheckPredicate, 36, 147, 3, // Skip to: 1847
/* 932 */     MCD_OPC_Decode, 168, 3, 235, 1, // Opcode: CMP_F_S
/* 937 */     MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 950
/* 941 */     MCD_OPC_CheckPredicate, 36, 134, 3, // Skip to: 1847
/* 945 */     MCD_OPC_Decode, 198, 3, 235, 1, // Opcode: CMP_UN_S
/* 950 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 963
/* 954 */     MCD_OPC_CheckPredicate, 36, 121, 3, // Skip to: 1847
/* 958 */     MCD_OPC_Decode, 166, 3, 235, 1, // Opcode: CMP_EQ_S
/* 963 */     MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 976
/* 967 */     MCD_OPC_CheckPredicate, 36, 108, 3, // Skip to: 1847
/* 971 */     MCD_OPC_Decode, 192, 3, 235, 1, // Opcode: CMP_UEQ_S
/* 976 */     MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 989
/* 980 */     MCD_OPC_CheckPredicate, 36, 95, 3, // Skip to: 1847
/* 984 */     MCD_OPC_Decode, 174, 3, 235, 1, // Opcode: CMP_LT_S
/* 989 */     MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 1002
/* 993 */     MCD_OPC_CheckPredicate, 36, 82, 3, // Skip to: 1847
/* 997 */     MCD_OPC_Decode, 196, 3, 235, 1, // Opcode: CMP_ULT_S
/* 1002 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 1015
/* 1006 */    MCD_OPC_CheckPredicate, 36, 69, 3, // Skip to: 1847
/* 1010 */    MCD_OPC_Decode, 171, 3, 235, 1, // Opcode: CMP_LE_S
/* 1015 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 1028
/* 1019 */    MCD_OPC_CheckPredicate, 36, 56, 3, // Skip to: 1847
/* 1023 */    MCD_OPC_Decode, 194, 3, 235, 1, // Opcode: CMP_ULE_S
/* 1028 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 1041
/* 1032 */    MCD_OPC_CheckPredicate, 36, 43, 3, // Skip to: 1847
/* 1036 */    MCD_OPC_Decode, 176, 3, 235, 1, // Opcode: CMP_SAF_S
/* 1041 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 1054
/* 1045 */    MCD_OPC_CheckPredicate, 36, 30, 3, // Skip to: 1847
/* 1049 */    MCD_OPC_Decode, 190, 3, 235, 1, // Opcode: CMP_SUN_S
/* 1054 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 1067
/* 1058 */    MCD_OPC_CheckPredicate, 36, 17, 3, // Skip to: 1847
/* 1062 */    MCD_OPC_Decode, 178, 3, 235, 1, // Opcode: CMP_SEQ_S
/* 1067 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 1080
/* 1071 */    MCD_OPC_CheckPredicate, 36, 4, 3, // Skip to: 1847
/* 1075 */    MCD_OPC_Decode, 184, 3, 235, 1, // Opcode: CMP_SUEQ_S
/* 1080 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 1093
/* 1084 */    MCD_OPC_CheckPredicate, 36, 247, 2, // Skip to: 1847
/* 1088 */    MCD_OPC_Decode, 182, 3, 235, 1, // Opcode: CMP_SLT_S
/* 1093 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 1106
/* 1097 */    MCD_OPC_CheckPredicate, 36, 234, 2, // Skip to: 1847
/* 1101 */    MCD_OPC_Decode, 188, 3, 235, 1, // Opcode: CMP_SULT_S
/* 1106 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 1119
/* 1110 */    MCD_OPC_CheckPredicate, 36, 221, 2, // Skip to: 1847
/* 1114 */    MCD_OPC_Decode, 180, 3, 235, 1, // Opcode: CMP_SLE_S
/* 1119 */    MCD_OPC_FilterValue, 15, 212, 2, // Skip to: 1847
/* 1123 */    MCD_OPC_CheckPredicate, 36, 208, 2, // Skip to: 1847
/* 1127 */    MCD_OPC_Decode, 186, 3, 235, 1, // Opcode: CMP_SULE_S
/* 1132 */    MCD_OPC_FilterValue, 21, 199, 2, // Skip to: 1847
/* 1136 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 1139 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 1152
/* 1143 */    MCD_OPC_CheckPredicate, 36, 188, 2, // Skip to: 1847
/* 1147 */    MCD_OPC_Decode, 167, 3, 236, 1, // Opcode: CMP_F_D
/* 1152 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 1165
/* 1156 */    MCD_OPC_CheckPredicate, 36, 175, 2, // Skip to: 1847
/* 1160 */    MCD_OPC_Decode, 197, 3, 236, 1, // Opcode: CMP_UN_D
/* 1165 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 1178
/* 1169 */    MCD_OPC_CheckPredicate, 36, 162, 2, // Skip to: 1847
/* 1173 */    MCD_OPC_Decode, 164, 3, 236, 1, // Opcode: CMP_EQ_D
/* 1178 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 1191
/* 1182 */    MCD_OPC_CheckPredicate, 36, 149, 2, // Skip to: 1847
/* 1186 */    MCD_OPC_Decode, 191, 3, 236, 1, // Opcode: CMP_UEQ_D
/* 1191 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 1204
/* 1195 */    MCD_OPC_CheckPredicate, 36, 136, 2, // Skip to: 1847
/* 1199 */    MCD_OPC_Decode, 172, 3, 236, 1, // Opcode: CMP_LT_D
/* 1204 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 1217
/* 1208 */    MCD_OPC_CheckPredicate, 36, 123, 2, // Skip to: 1847
/* 1212 */    MCD_OPC_Decode, 195, 3, 236, 1, // Opcode: CMP_ULT_D
/* 1217 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 1230
/* 1221 */    MCD_OPC_CheckPredicate, 36, 110, 2, // Skip to: 1847
/* 1225 */    MCD_OPC_Decode, 169, 3, 236, 1, // Opcode: CMP_LE_D
/* 1230 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 1243
/* 1234 */    MCD_OPC_CheckPredicate, 36, 97, 2, // Skip to: 1847
/* 1238 */    MCD_OPC_Decode, 193, 3, 236, 1, // Opcode: CMP_ULE_D
/* 1243 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 1256
/* 1247 */    MCD_OPC_CheckPredicate, 36, 84, 2, // Skip to: 1847
/* 1251 */    MCD_OPC_Decode, 175, 3, 236, 1, // Opcode: CMP_SAF_D
/* 1256 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 1269
/* 1260 */    MCD_OPC_CheckPredicate, 36, 71, 2, // Skip to: 1847
/* 1264 */    MCD_OPC_Decode, 189, 3, 236, 1, // Opcode: CMP_SUN_D
/* 1269 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 1282
/* 1273 */    MCD_OPC_CheckPredicate, 36, 58, 2, // Skip to: 1847
/* 1277 */    MCD_OPC_Decode, 177, 3, 236, 1, // Opcode: CMP_SEQ_D
/* 1282 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 1295
/* 1286 */    MCD_OPC_CheckPredicate, 36, 45, 2, // Skip to: 1847
/* 1290 */    MCD_OPC_Decode, 183, 3, 236, 1, // Opcode: CMP_SUEQ_D
/* 1295 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 1308
/* 1299 */    MCD_OPC_CheckPredicate, 36, 32, 2, // Skip to: 1847
/* 1303 */    MCD_OPC_Decode, 181, 3, 236, 1, // Opcode: CMP_SLT_D
/* 1308 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 1321
/* 1312 */    MCD_OPC_CheckPredicate, 36, 19, 2, // Skip to: 1847
/* 1316 */    MCD_OPC_Decode, 187, 3, 236, 1, // Opcode: CMP_SULT_D
/* 1321 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 1334
/* 1325 */    MCD_OPC_CheckPredicate, 36, 6, 2, // Skip to: 1847
/* 1329 */    MCD_OPC_Decode, 179, 3, 236, 1, // Opcode: CMP_SLE_D
/* 1334 */    MCD_OPC_FilterValue, 15, 253, 1, // Skip to: 1847
/* 1338 */    MCD_OPC_CheckPredicate, 36, 249, 1, // Skip to: 1847
/* 1342 */    MCD_OPC_Decode, 185, 3, 236, 1, // Opcode: CMP_SULE_D
/* 1347 */    MCD_OPC_FilterValue, 18, 81, 0, // Skip to: 1432
/* 1351 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 1354 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 1367
/* 1358 */    MCD_OPC_CheckPredicate, 36, 229, 1, // Skip to: 1847
/* 1362 */    MCD_OPC_Decode, 188, 1, 237, 1, // Opcode: BC2EQZ
/* 1367 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 1380
/* 1371 */    MCD_OPC_CheckPredicate, 36, 216, 1, // Skip to: 1847
/* 1375 */    MCD_OPC_Decode, 219, 7, 238, 1, // Opcode: LWC2_R6
/* 1380 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 1393
/* 1384 */    MCD_OPC_CheckPredicate, 36, 203, 1, // Skip to: 1847
/* 1388 */    MCD_OPC_Decode, 241, 12, 238, 1, // Opcode: SWC2_R6
/* 1393 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 1406
/* 1397 */    MCD_OPC_CheckPredicate, 36, 190, 1, // Skip to: 1847
/* 1401 */    MCD_OPC_Decode, 191, 1, 237, 1, // Opcode: BC2NEZ
/* 1406 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 1419
/* 1410 */    MCD_OPC_CheckPredicate, 36, 177, 1, // Skip to: 1847
/* 1414 */    MCD_OPC_Decode, 166, 7, 238, 1, // Opcode: LDC2_R6
/* 1419 */    MCD_OPC_FilterValue, 15, 168, 1, // Skip to: 1847
/* 1423 */    MCD_OPC_CheckPredicate, 36, 164, 1, // Skip to: 1847
/* 1427 */    MCD_OPC_Decode, 160, 11, 238, 1, // Opcode: SDC2_R6
/* 1432 */    MCD_OPC_FilterValue, 22, 9, 0, // Skip to: 1445
/* 1436 */    MCD_OPC_CheckPredicate, 36, 151, 1, // Skip to: 1847
/* 1440 */    MCD_OPC_Decode, 224, 1, 239, 1, // Opcode: BGEZC
/* 1445 */    MCD_OPC_FilterValue, 23, 9, 0, // Skip to: 1458
/* 1449 */    MCD_OPC_CheckPredicate, 36, 138, 1, // Skip to: 1847
/* 1453 */    MCD_OPC_Decode, 138, 2, 240, 1, // Opcode: BLTZC
/* 1458 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 1471
/* 1462 */    MCD_OPC_CheckPredicate, 36, 125, 1, // Skip to: 1847
/* 1466 */    MCD_OPC_Decode, 147, 2, 241, 1, // Opcode: BNEC
/* 1471 */    MCD_OPC_FilterValue, 29, 9, 0, // Skip to: 1484
/* 1475 */    MCD_OPC_CheckPredicate, 37, 112, 1, // Skip to: 1847
/* 1479 */    MCD_OPC_Decode, 166, 4, 242, 1, // Opcode: DAUI
/* 1484 */    MCD_OPC_FilterValue, 31, 182, 0, // Skip to: 1670
/* 1488 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 1491 */    MCD_OPC_FilterValue, 32, 40, 0, // Skip to: 1535
/* 1495 */    MCD_OPC_ExtractField, 8, 3,  // Inst{10-8} ...
/* 1498 */    MCD_OPC_FilterValue, 0, 21, 0, // Skip to: 1523
/* 1502 */    MCD_OPC_CheckPredicate, 36, 85, 1, // Skip to: 1847
/* 1506 */    MCD_OPC_CheckField, 21, 5, 0, 79, 1, // Skip to: 1847
/* 1512 */    MCD_OPC_CheckField, 6, 2, 0, 73, 1, // Skip to: 1847
/* 1518 */    MCD_OPC_Decode, 250, 1, 205, 1, // Opcode: BITSWAP
/* 1523 */    MCD_OPC_FilterValue, 2, 64, 1, // Skip to: 1847
/* 1527 */    MCD_OPC_CheckPredicate, 36, 60, 1, // Skip to: 1847
/* 1531 */    MCD_OPC_Decode, 81, 221, 1, // Opcode: ALIGN
/* 1535 */    MCD_OPC_FilterValue, 36, 41, 0, // Skip to: 1580
/* 1539 */    MCD_OPC_ExtractField, 9, 2,  // Inst{10-9} ...
/* 1542 */    MCD_OPC_FilterValue, 0, 21, 0, // Skip to: 1567
/* 1546 */    MCD_OPC_CheckPredicate, 37, 41, 1, // Skip to: 1847
/* 1550 */    MCD_OPC_CheckField, 21, 5, 0, 35, 1, // Skip to: 1847
/* 1556 */    MCD_OPC_CheckField, 6, 3, 0, 29, 1, // Skip to: 1847
/* 1562 */    MCD_OPC_Decode, 167, 4, 243, 1, // Opcode: DBITSWAP
/* 1567 */    MCD_OPC_FilterValue, 1, 20, 1, // Skip to: 1847
/* 1571 */    MCD_OPC_CheckPredicate, 37, 16, 1, // Skip to: 1847
/* 1575 */    MCD_OPC_Decode, 164, 4, 244, 1, // Opcode: DALIGN
/* 1580 */    MCD_OPC_FilterValue, 37, 15, 0, // Skip to: 1599
/* 1584 */    MCD_OPC_CheckPredicate, 36, 3, 1, // Skip to: 1847
/* 1588 */    MCD_OPC_CheckField, 6, 1, 0, 253, 0, // Skip to: 1847
/* 1594 */    MCD_OPC_Decode, 222, 2, 245, 1, // Opcode: CACHE_R6
/* 1599 */    MCD_OPC_FilterValue, 38, 9, 0, // Skip to: 1612
/* 1603 */    MCD_OPC_CheckPredicate, 36, 240, 0, // Skip to: 1847
/* 1607 */    MCD_OPC_Decode, 150, 11, 246, 1, // Opcode: SC_R6
/* 1612 */    MCD_OPC_FilterValue, 39, 9, 0, // Skip to: 1625
/* 1616 */    MCD_OPC_CheckPredicate, 36, 227, 0, // Skip to: 1847
/* 1620 */    MCD_OPC_Decode, 148, 11, 246, 1, // Opcode: SCD_R6
/* 1625 */    MCD_OPC_FilterValue, 53, 15, 0, // Skip to: 1644
/* 1629 */    MCD_OPC_CheckPredicate, 36, 214, 0, // Skip to: 1847
/* 1633 */    MCD_OPC_CheckField, 6, 1, 0, 208, 0, // Skip to: 1847
/* 1639 */    MCD_OPC_Decode, 183, 10, 245, 1, // Opcode: PREF_R6
/* 1644 */    MCD_OPC_FilterValue, 54, 9, 0, // Skip to: 1657
/* 1648 */    MCD_OPC_CheckPredicate, 36, 195, 0, // Skip to: 1847
/* 1652 */    MCD_OPC_Decode, 197, 7, 246, 1, // Opcode: LL_R6
/* 1657 */    MCD_OPC_FilterValue, 55, 186, 0, // Skip to: 1847
/* 1661 */    MCD_OPC_CheckPredicate, 36, 182, 0, // Skip to: 1847
/* 1665 */    MCD_OPC_Decode, 195, 7, 246, 1, // Opcode: LLD_R6
/* 1670 */    MCD_OPC_FilterValue, 50, 9, 0, // Skip to: 1683
/* 1674 */    MCD_OPC_CheckPredicate, 36, 169, 0, // Skip to: 1847
/* 1678 */    MCD_OPC_Decode, 175, 1, 247, 1, // Opcode: BC
/* 1683 */    MCD_OPC_FilterValue, 54, 23, 0, // Skip to: 1710
/* 1687 */    MCD_OPC_CheckPredicate, 36, 10, 0, // Skip to: 1701
/* 1691 */    MCD_OPC_CheckField, 21, 5, 0, 4, 0, // Skip to: 1701
/* 1697 */    MCD_OPC_Decode, 135, 7, 52, // Opcode: JIC
/* 1701 */    MCD_OPC_CheckPredicate, 36, 142, 0, // Skip to: 1847
/* 1705 */    MCD_OPC_Decode, 212, 1, 248, 1, // Opcode: BEQZC
/* 1710 */    MCD_OPC_FilterValue, 58, 9, 0, // Skip to: 1723
/* 1714 */    MCD_OPC_CheckPredicate, 36, 129, 0, // Skip to: 1847
/* 1718 */    MCD_OPC_Decode, 168, 1, 247, 1, // Opcode: BALC
/* 1723 */    MCD_OPC_FilterValue, 59, 93, 0, // Skip to: 1820
/* 1727 */    MCD_OPC_ExtractField, 19, 2,  // Inst{20-19} ...
/* 1730 */    MCD_OPC_FilterValue, 0, 8, 0, // Skip to: 1742
/* 1734 */    MCD_OPC_CheckPredicate, 36, 109, 0, // Skip to: 1847
/* 1738 */    MCD_OPC_Decode, 26, 249, 1, // Opcode: ADDIUPC
/* 1742 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 1755
/* 1746 */    MCD_OPC_CheckPredicate, 36, 97, 0, // Skip to: 1847
/* 1750 */    MCD_OPC_Decode, 228, 7, 249, 1, // Opcode: LWPC
/* 1755 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 1768
/* 1759 */    MCD_OPC_CheckPredicate, 36, 84, 0, // Skip to: 1847
/* 1763 */    MCD_OPC_Decode, 234, 7, 249, 1, // Opcode: LWUPC
/* 1768 */    MCD_OPC_FilterValue, 3, 75, 0, // Skip to: 1847
/* 1772 */    MCD_OPC_ExtractField, 18, 1,  // Inst{18} ...
/* 1775 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 1788
/* 1779 */    MCD_OPC_CheckPredicate, 37, 64, 0, // Skip to: 1847
/* 1783 */    MCD_OPC_Decode, 173, 7, 250, 1, // Opcode: LDPC
/* 1788 */    MCD_OPC_FilterValue, 1, 55, 0, // Skip to: 1847
/* 1792 */    MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 1795 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 1808
/* 1799 */    MCD_OPC_CheckPredicate, 36, 44, 0, // Skip to: 1847
/* 1803 */    MCD_OPC_Decode, 138, 1, 251, 1, // Opcode: AUIPC
/* 1808 */    MCD_OPC_FilterValue, 3, 35, 0, // Skip to: 1847
/* 1812 */    MCD_OPC_CheckPredicate, 36, 31, 0, // Skip to: 1847
/* 1816 */    MCD_OPC_Decode, 82, 251, 1, // Opcode: ALUIPC
/* 1820 */    MCD_OPC_FilterValue, 62, 23, 0, // Skip to: 1847
/* 1824 */    MCD_OPC_CheckPredicate, 36, 10, 0, // Skip to: 1838
/* 1828 */    MCD_OPC_CheckField, 21, 5, 0, 4, 0, // Skip to: 1838
/* 1834 */    MCD_OPC_Decode, 134, 7, 52, // Opcode: JIALC
/* 1838 */    MCD_OPC_CheckPredicate, 36, 5, 0, // Skip to: 1847
/* 1842 */    MCD_OPC_Decode, 159, 2, 248, 1, // Opcode: BNEZC
/* 1847 */    MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTableMips32r6_64r6_GP6432[] = {
/* 0 */       MCD_OPC_ExtractField, 0, 11,  // Inst{10-0} ...
/* 3 */       MCD_OPC_FilterValue, 53, 15, 0, // Skip to: 22
/* 7 */       MCD_OPC_CheckPredicate, 39, 30, 0, // Skip to: 41
/* 11 */      MCD_OPC_CheckField, 26, 6, 0, 24, 0, // Skip to: 41
/* 17 */      MCD_OPC_Decode, 175, 11, 224, 1, // Opcode: SELEQZ64
/* 22 */      MCD_OPC_FilterValue, 55, 15, 0, // Skip to: 41
/* 26 */      MCD_OPC_CheckPredicate, 39, 11, 0, // Skip to: 41
/* 30 */      MCD_OPC_CheckField, 26, 6, 0, 5, 0, // Skip to: 41
/* 36 */      MCD_OPC_Decode, 179, 11, 224, 1, // Opcode: SELNEZ64
/* 41 */      MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTableMips6432[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 0, 112, 1, // Skip to: 375
/* 7 */       MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 10 */      MCD_OPC_FilterValue, 20, 15, 0, // Skip to: 29
/* 14 */      MCD_OPC_CheckPredicate, 19, 42, 9, // Skip to: 2364
/* 18 */      MCD_OPC_CheckField, 6, 5, 0, 36, 9, // Skip to: 2364
/* 24 */      MCD_OPC_Decode, 255, 4, 252, 1, // Opcode: DSLLV
/* 29 */      MCD_OPC_FilterValue, 22, 29, 0, // Skip to: 62
/* 33 */      MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 36 */      MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 49
/* 40 */      MCD_OPC_CheckPredicate, 19, 16, 9, // Skip to: 2364
/* 44 */      MCD_OPC_Decode, 133, 5, 252, 1, // Opcode: DSRLV
/* 49 */      MCD_OPC_FilterValue, 1, 7, 9, // Skip to: 2364
/* 53 */      MCD_OPC_CheckPredicate, 40, 3, 9, // Skip to: 2364
/* 57 */      MCD_OPC_Decode, 248, 4, 252, 1, // Opcode: DROTRV
/* 62 */      MCD_OPC_FilterValue, 23, 15, 0, // Skip to: 81
/* 66 */      MCD_OPC_CheckPredicate, 19, 246, 8, // Skip to: 2364
/* 70 */      MCD_OPC_CheckField, 6, 5, 0, 240, 8, // Skip to: 2364
/* 76 */      MCD_OPC_Decode, 130, 5, 252, 1, // Opcode: DSRAV
/* 81 */      MCD_OPC_FilterValue, 28, 15, 0, // Skip to: 100
/* 85 */      MCD_OPC_CheckPredicate, 41, 227, 8, // Skip to: 2364
/* 89 */      MCD_OPC_CheckField, 6, 10, 0, 221, 8, // Skip to: 2364
/* 95 */      MCD_OPC_Decode, 207, 4, 253, 1, // Opcode: DMULT
/* 100 */     MCD_OPC_FilterValue, 29, 15, 0, // Skip to: 119
/* 104 */     MCD_OPC_CheckPredicate, 41, 208, 8, // Skip to: 2364
/* 108 */     MCD_OPC_CheckField, 6, 10, 0, 202, 8, // Skip to: 2364
/* 114 */     MCD_OPC_Decode, 208, 4, 253, 1, // Opcode: DMULTu
/* 119 */     MCD_OPC_FilterValue, 30, 15, 0, // Skip to: 138
/* 123 */     MCD_OPC_CheckPredicate, 41, 189, 8, // Skip to: 2364
/* 127 */     MCD_OPC_CheckField, 6, 10, 0, 183, 8, // Skip to: 2364
/* 133 */     MCD_OPC_Decode, 250, 4, 253, 1, // Opcode: DSDIV
/* 138 */     MCD_OPC_FilterValue, 31, 15, 0, // Skip to: 157
/* 142 */     MCD_OPC_CheckPredicate, 41, 170, 8, // Skip to: 2364
/* 146 */     MCD_OPC_CheckField, 6, 10, 0, 164, 8, // Skip to: 2364
/* 152 */     MCD_OPC_Decode, 136, 5, 253, 1, // Opcode: DUDIV
/* 157 */     MCD_OPC_FilterValue, 44, 15, 0, // Skip to: 176
/* 161 */     MCD_OPC_CheckPredicate, 19, 151, 8, // Skip to: 2364
/* 165 */     MCD_OPC_CheckField, 6, 5, 0, 145, 8, // Skip to: 2364
/* 171 */     MCD_OPC_Decode, 159, 4, 224, 1, // Opcode: DADD
/* 176 */     MCD_OPC_FilterValue, 45, 15, 0, // Skip to: 195
/* 180 */     MCD_OPC_CheckPredicate, 19, 132, 8, // Skip to: 2364
/* 184 */     MCD_OPC_CheckField, 6, 5, 0, 126, 8, // Skip to: 2364
/* 190 */     MCD_OPC_Decode, 162, 4, 224, 1, // Opcode: DADDu
/* 195 */     MCD_OPC_FilterValue, 46, 15, 0, // Skip to: 214
/* 199 */     MCD_OPC_CheckPredicate, 19, 113, 8, // Skip to: 2364
/* 203 */     MCD_OPC_CheckField, 6, 5, 0, 107, 8, // Skip to: 2364
/* 209 */     MCD_OPC_Decode, 134, 5, 224, 1, // Opcode: DSUB
/* 214 */     MCD_OPC_FilterValue, 47, 15, 0, // Skip to: 233
/* 218 */     MCD_OPC_CheckPredicate, 19, 94, 8, // Skip to: 2364
/* 222 */     MCD_OPC_CheckField, 6, 5, 0, 88, 8, // Skip to: 2364
/* 228 */     MCD_OPC_Decode, 135, 5, 224, 1, // Opcode: DSUBu
/* 233 */     MCD_OPC_FilterValue, 56, 15, 0, // Skip to: 252
/* 237 */     MCD_OPC_CheckPredicate, 19, 75, 8, // Skip to: 2364
/* 241 */     MCD_OPC_CheckField, 21, 5, 0, 69, 8, // Skip to: 2364
/* 247 */     MCD_OPC_Decode, 252, 4, 254, 1, // Opcode: DSLL
/* 252 */     MCD_OPC_FilterValue, 58, 29, 0, // Skip to: 285
/* 256 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 259 */     MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 272
/* 263 */     MCD_OPC_CheckPredicate, 19, 49, 8, // Skip to: 2364
/* 267 */     MCD_OPC_Decode, 131, 5, 254, 1, // Opcode: DSRL
/* 272 */     MCD_OPC_FilterValue, 1, 40, 8, // Skip to: 2364
/* 276 */     MCD_OPC_CheckPredicate, 40, 36, 8, // Skip to: 2364
/* 280 */     MCD_OPC_Decode, 246, 4, 254, 1, // Opcode: DROTR
/* 285 */     MCD_OPC_FilterValue, 59, 15, 0, // Skip to: 304
/* 289 */     MCD_OPC_CheckPredicate, 19, 23, 8, // Skip to: 2364
/* 293 */     MCD_OPC_CheckField, 21, 5, 0, 17, 8, // Skip to: 2364
/* 299 */     MCD_OPC_Decode, 128, 5, 254, 1, // Opcode: DSRA
/* 304 */     MCD_OPC_FilterValue, 60, 15, 0, // Skip to: 323
/* 308 */     MCD_OPC_CheckPredicate, 19, 4, 8, // Skip to: 2364
/* 312 */     MCD_OPC_CheckField, 21, 5, 0, 254, 7, // Skip to: 2364
/* 318 */     MCD_OPC_Decode, 253, 4, 254, 1, // Opcode: DSLL32
/* 323 */     MCD_OPC_FilterValue, 62, 29, 0, // Skip to: 356
/* 327 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 330 */     MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 343
/* 334 */     MCD_OPC_CheckPredicate, 19, 234, 7, // Skip to: 2364
/* 338 */     MCD_OPC_Decode, 132, 5, 254, 1, // Opcode: DSRL32
/* 343 */     MCD_OPC_FilterValue, 1, 225, 7, // Skip to: 2364
/* 347 */     MCD_OPC_CheckPredicate, 40, 221, 7, // Skip to: 2364
/* 351 */     MCD_OPC_Decode, 247, 4, 254, 1, // Opcode: DROTR32
/* 356 */     MCD_OPC_FilterValue, 63, 212, 7, // Skip to: 2364
/* 360 */     MCD_OPC_CheckPredicate, 19, 208, 7, // Skip to: 2364
/* 364 */     MCD_OPC_CheckField, 21, 5, 0, 202, 7, // Skip to: 2364
/* 370 */     MCD_OPC_Decode, 129, 5, 254, 1, // Opcode: DSRA32
/* 375 */     MCD_OPC_FilterValue, 16, 41, 0, // Skip to: 420
/* 379 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 382 */     MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 401
/* 386 */     MCD_OPC_CheckPredicate, 42, 182, 7, // Skip to: 2364
/* 390 */     MCD_OPC_CheckField, 3, 8, 0, 176, 7, // Skip to: 2364
/* 396 */     MCD_OPC_Decode, 196, 4, 255, 1, // Opcode: DMFC0
/* 401 */     MCD_OPC_FilterValue, 5, 167, 7, // Skip to: 2364
/* 405 */     MCD_OPC_CheckPredicate, 42, 163, 7, // Skip to: 2364
/* 409 */     MCD_OPC_CheckField, 3, 8, 0, 157, 7, // Skip to: 2364
/* 415 */     MCD_OPC_Decode, 201, 4, 255, 1, // Opcode: DMTC0
/* 420 */     MCD_OPC_FilterValue, 17, 222, 3, // Skip to: 1414
/* 424 */     MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 427 */     MCD_OPC_FilterValue, 0, 54, 0, // Skip to: 485
/* 431 */     MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 434 */     MCD_OPC_FilterValue, 3, 15, 0, // Skip to: 453
/* 438 */     MCD_OPC_CheckPredicate, 43, 130, 7, // Skip to: 2364
/* 442 */     MCD_OPC_CheckField, 6, 5, 0, 124, 7, // Skip to: 2364
/* 448 */     MCD_OPC_Decode, 184, 8, 128, 2, // Opcode: MFHC1_D64
/* 453 */     MCD_OPC_FilterValue, 7, 15, 0, // Skip to: 472
/* 457 */     MCD_OPC_CheckPredicate, 43, 111, 7, // Skip to: 2364
/* 461 */     MCD_OPC_CheckField, 6, 5, 0, 105, 7, // Skip to: 2364
/* 467 */     MCD_OPC_Decode, 174, 9, 129, 2, // Opcode: MTHC1_D64
/* 472 */     MCD_OPC_FilterValue, 17, 96, 7, // Skip to: 2364
/* 476 */     MCD_OPC_CheckPredicate, 44, 92, 7, // Skip to: 2364
/* 480 */     MCD_OPC_Decode, 172, 5, 233, 1, // Opcode: FADD_D64
/* 485 */     MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 504
/* 489 */     MCD_OPC_CheckPredicate, 44, 79, 7, // Skip to: 2364
/* 493 */     MCD_OPC_CheckField, 21, 5, 17, 73, 7, // Skip to: 2364
/* 499 */     MCD_OPC_Decode, 174, 6, 233, 1, // Opcode: FSUB_D64
/* 504 */     MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 523
/* 508 */     MCD_OPC_CheckPredicate, 44, 60, 7, // Skip to: 2364
/* 512 */     MCD_OPC_CheckField, 21, 5, 17, 54, 7, // Skip to: 2364
/* 518 */     MCD_OPC_Decode, 137, 6, 233, 1, // Opcode: FMUL_D64
/* 523 */     MCD_OPC_FilterValue, 3, 15, 0, // Skip to: 542
/* 527 */     MCD_OPC_CheckPredicate, 44, 41, 7, // Skip to: 2364
/* 531 */     MCD_OPC_CheckField, 21, 5, 17, 35, 7, // Skip to: 2364
/* 537 */     MCD_OPC_Decode, 208, 5, 233, 1, // Opcode: FDIV_D64
/* 542 */     MCD_OPC_FilterValue, 4, 15, 0, // Skip to: 561
/* 546 */     MCD_OPC_CheckPredicate, 45, 22, 7, // Skip to: 2364
/* 550 */     MCD_OPC_CheckField, 16, 10, 160, 4, 15, 7, // Skip to: 2364
/* 557 */     MCD_OPC_Decode, 167, 6, 105, // Opcode: FSQRT_D64
/* 561 */     MCD_OPC_FilterValue, 5, 15, 0, // Skip to: 580
/* 565 */     MCD_OPC_CheckPredicate, 44, 3, 7, // Skip to: 2364
/* 569 */     MCD_OPC_CheckField, 16, 10, 160, 4, 252, 6, // Skip to: 2364
/* 576 */     MCD_OPC_Decode, 165, 5, 105, // Opcode: FABS_D64
/* 580 */     MCD_OPC_FilterValue, 6, 15, 0, // Skip to: 599
/* 584 */     MCD_OPC_CheckPredicate, 44, 240, 6, // Skip to: 2364
/* 588 */     MCD_OPC_CheckField, 16, 10, 160, 4, 233, 6, // Skip to: 2364
/* 595 */     MCD_OPC_Decode, 130, 6, 105, // Opcode: FMOV_D64
/* 599 */     MCD_OPC_FilterValue, 7, 15, 0, // Skip to: 618
/* 603 */     MCD_OPC_CheckPredicate, 44, 221, 6, // Skip to: 2364
/* 607 */     MCD_OPC_CheckField, 16, 10, 160, 4, 214, 6, // Skip to: 2364
/* 614 */     MCD_OPC_Decode, 143, 6, 105, // Opcode: FNEG_D64
/* 618 */     MCD_OPC_FilterValue, 8, 29, 0, // Skip to: 651
/* 622 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 625 */     MCD_OPC_FilterValue, 128, 4, 8, 0, // Skip to: 638
/* 630 */     MCD_OPC_CheckPredicate, 44, 194, 6, // Skip to: 2364
/* 634 */     MCD_OPC_Decode, 252, 10, 98, // Opcode: ROUND_L_S
/* 638 */     MCD_OPC_FilterValue, 160, 4, 185, 6, // Skip to: 2364
/* 643 */     MCD_OPC_CheckPredicate, 44, 181, 6, // Skip to: 2364
/* 647 */     MCD_OPC_Decode, 251, 10, 105, // Opcode: ROUND_L_D64
/* 651 */     MCD_OPC_FilterValue, 9, 29, 0, // Skip to: 684
/* 655 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 658 */     MCD_OPC_FilterValue, 128, 4, 8, 0, // Skip to: 671
/* 663 */     MCD_OPC_CheckPredicate, 44, 161, 6, // Skip to: 2364
/* 667 */     MCD_OPC_Decode, 215, 13, 98, // Opcode: TRUNC_L_S
/* 671 */     MCD_OPC_FilterValue, 160, 4, 152, 6, // Skip to: 2364
/* 676 */     MCD_OPC_CheckPredicate, 44, 148, 6, // Skip to: 2364
/* 680 */     MCD_OPC_Decode, 214, 13, 105, // Opcode: TRUNC_L_D64
/* 684 */     MCD_OPC_FilterValue, 10, 29, 0, // Skip to: 717
/* 688 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 691 */     MCD_OPC_FilterValue, 128, 4, 8, 0, // Skip to: 704
/* 696 */     MCD_OPC_CheckPredicate, 44, 128, 6, // Skip to: 2364
/* 700 */     MCD_OPC_Decode, 224, 2, 98, // Opcode: CEIL_L_S
/* 704 */     MCD_OPC_FilterValue, 160, 4, 119, 6, // Skip to: 2364
/* 709 */     MCD_OPC_CheckPredicate, 44, 115, 6, // Skip to: 2364
/* 713 */     MCD_OPC_Decode, 223, 2, 105, // Opcode: CEIL_L_D64
/* 717 */     MCD_OPC_FilterValue, 11, 29, 0, // Skip to: 750
/* 721 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 724 */     MCD_OPC_FilterValue, 128, 4, 8, 0, // Skip to: 737
/* 729 */     MCD_OPC_CheckPredicate, 44, 95, 6, // Skip to: 2364
/* 733 */     MCD_OPC_Decode, 240, 5, 98, // Opcode: FLOOR_L_S
/* 737 */     MCD_OPC_FilterValue, 160, 4, 86, 6, // Skip to: 2364
/* 742 */     MCD_OPC_CheckPredicate, 44, 82, 6, // Skip to: 2364
/* 746 */     MCD_OPC_Decode, 239, 5, 105, // Opcode: FLOOR_L_D64
/* 750 */     MCD_OPC_FilterValue, 12, 16, 0, // Skip to: 770
/* 754 */     MCD_OPC_CheckPredicate, 45, 70, 6, // Skip to: 2364
/* 758 */     MCD_OPC_CheckField, 16, 10, 160, 4, 63, 6, // Skip to: 2364
/* 765 */     MCD_OPC_Decode, 254, 10, 130, 2, // Opcode: ROUND_W_D64
/* 770 */     MCD_OPC_FilterValue, 13, 16, 0, // Skip to: 790
/* 774 */     MCD_OPC_CheckPredicate, 45, 50, 6, // Skip to: 2364
/* 778 */     MCD_OPC_CheckField, 16, 10, 160, 4, 43, 6, // Skip to: 2364
/* 785 */     MCD_OPC_Decode, 217, 13, 130, 2, // Opcode: TRUNC_W_D64
/* 790 */     MCD_OPC_FilterValue, 14, 16, 0, // Skip to: 810
/* 794 */     MCD_OPC_CheckPredicate, 45, 30, 6, // Skip to: 2364
/* 798 */     MCD_OPC_CheckField, 16, 10, 160, 4, 23, 6, // Skip to: 2364
/* 805 */     MCD_OPC_Decode, 226, 2, 130, 2, // Opcode: CEIL_W_D64
/* 810 */     MCD_OPC_FilterValue, 15, 16, 0, // Skip to: 830
/* 814 */     MCD_OPC_CheckPredicate, 45, 10, 6, // Skip to: 2364
/* 818 */     MCD_OPC_CheckField, 16, 10, 160, 4, 3, 6, // Skip to: 2364
/* 825 */     MCD_OPC_Decode, 242, 5, 130, 2, // Opcode: FLOOR_W_D64
/* 830 */     MCD_OPC_FilterValue, 17, 41, 0, // Skip to: 875
/* 834 */     MCD_OPC_ExtractField, 16, 2,  // Inst{17-16} ...
/* 837 */     MCD_OPC_FilterValue, 0, 15, 0, // Skip to: 856
/* 841 */     MCD_OPC_CheckPredicate, 46, 239, 5, // Skip to: 2364
/* 845 */     MCD_OPC_CheckField, 21, 5, 17, 233, 5, // Skip to: 2364
/* 851 */     MCD_OPC_Decode, 238, 8, 131, 2, // Opcode: MOVF_D64
/* 856 */     MCD_OPC_FilterValue, 1, 224, 5, // Skip to: 2364
/* 860 */     MCD_OPC_CheckPredicate, 46, 220, 5, // Skip to: 2364
/* 864 */     MCD_OPC_CheckField, 21, 5, 17, 214, 5, // Skip to: 2364
/* 870 */     MCD_OPC_Decode, 130, 9, 131, 2, // Opcode: MOVT_D64
/* 875 */     MCD_OPC_FilterValue, 18, 15, 0, // Skip to: 894
/* 879 */     MCD_OPC_CheckPredicate, 46, 201, 5, // Skip to: 2364
/* 883 */     MCD_OPC_CheckField, 21, 5, 17, 195, 5, // Skip to: 2364
/* 889 */     MCD_OPC_Decode, 142, 9, 132, 2, // Opcode: MOVZ_I_D64
/* 894 */     MCD_OPC_FilterValue, 19, 15, 0, // Skip to: 913
/* 898 */     MCD_OPC_CheckPredicate, 46, 182, 5, // Skip to: 2364
/* 902 */     MCD_OPC_CheckField, 21, 5, 17, 176, 5, // Skip to: 2364
/* 908 */     MCD_OPC_Decode, 250, 8, 132, 2, // Opcode: MOVN_I_D64
/* 913 */     MCD_OPC_FilterValue, 32, 31, 0, // Skip to: 948
/* 917 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 920 */     MCD_OPC_FilterValue, 160, 4, 9, 0, // Skip to: 934
/* 925 */     MCD_OPC_CheckPredicate, 44, 155, 5, // Skip to: 2364
/* 929 */     MCD_OPC_Decode, 226, 3, 130, 2, // Opcode: CVT_S_D64
/* 934 */     MCD_OPC_FilterValue, 160, 5, 145, 5, // Skip to: 2364
/* 939 */     MCD_OPC_CheckPredicate, 44, 141, 5, // Skip to: 2364
/* 943 */     MCD_OPC_Decode, 227, 3, 130, 2, // Opcode: CVT_S_L
/* 948 */     MCD_OPC_FilterValue, 33, 42, 0, // Skip to: 994
/* 952 */     MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 955 */     MCD_OPC_FilterValue, 128, 4, 8, 0, // Skip to: 968
/* 960 */     MCD_OPC_CheckPredicate, 44, 120, 5, // Skip to: 2364
/* 964 */     MCD_OPC_Decode, 217, 3, 98, // Opcode: CVT_D64_S
/* 968 */     MCD_OPC_FilterValue, 128, 5, 8, 0, // Skip to: 981
/* 973 */     MCD_OPC_CheckPredicate, 44, 107, 5, // Skip to: 2364
/* 977 */     MCD_OPC_Decode, 218, 3, 98, // Opcode: CVT_D64_W
/* 981 */     MCD_OPC_FilterValue, 160, 5, 98, 5, // Skip to: 2364
/* 986 */     MCD_OPC_CheckPredicate, 44, 94, 5, // Skip to: 2364
/* 990 */     MCD_OPC_Decode, 216, 3, 105, // Opcode: CVT_D64_L
/* 994 */     MCD_OPC_FilterValue, 36, 16, 0, // Skip to: 1014
/* 998 */     MCD_OPC_CheckPredicate, 44, 82, 5, // Skip to: 2364
/* 1002 */    MCD_OPC_CheckField, 16, 10, 160, 4, 75, 5, // Skip to: 2364
/* 1009 */    MCD_OPC_Decode, 231, 3, 130, 2, // Opcode: CVT_W_D64
/* 1014 */    MCD_OPC_FilterValue, 48, 21, 0, // Skip to: 1039
/* 1018 */    MCD_OPC_CheckPredicate, 47, 62, 5, // Skip to: 2364
/* 1022 */    MCD_OPC_CheckField, 21, 5, 17, 56, 5, // Skip to: 2364
/* 1028 */    MCD_OPC_CheckField, 6, 5, 0, 50, 5, // Skip to: 2364
/* 1034 */    MCD_OPC_Decode, 239, 3, 133, 2, // Opcode: C_F_D64
/* 1039 */    MCD_OPC_FilterValue, 49, 21, 0, // Skip to: 1064
/* 1043 */    MCD_OPC_CheckPredicate, 47, 37, 5, // Skip to: 2364
/* 1047 */    MCD_OPC_CheckField, 21, 5, 17, 31, 5, // Skip to: 2364
/* 1053 */    MCD_OPC_CheckField, 6, 5, 0, 25, 5, // Skip to: 2364
/* 1059 */    MCD_OPC_Decode, 153, 4, 133, 2, // Opcode: C_UN_D64
/* 1064 */    MCD_OPC_FilterValue, 50, 21, 0, // Skip to: 1089
/* 1068 */    MCD_OPC_CheckPredicate, 47, 12, 5, // Skip to: 2364
/* 1072 */    MCD_OPC_CheckField, 21, 5, 17, 6, 5, // Skip to: 2364
/* 1078 */    MCD_OPC_CheckField, 6, 5, 0, 0, 5, // Skip to: 2364
/* 1084 */    MCD_OPC_Decode, 236, 3, 133, 2, // Opcode: C_EQ_D64
/* 1089 */    MCD_OPC_FilterValue, 51, 21, 0, // Skip to: 1114
/* 1093 */    MCD_OPC_CheckPredicate, 47, 243, 4, // Skip to: 2364
/* 1097 */    MCD_OPC_CheckField, 21, 5, 17, 237, 4, // Skip to: 2364
/* 1103 */    MCD_OPC_CheckField, 6, 5, 0, 231, 4, // Skip to: 2364
/* 1109 */    MCD_OPC_Decode, 144, 4, 133, 2, // Opcode: C_UEQ_D64
/* 1114 */    MCD_OPC_FilterValue, 52, 21, 0, // Skip to: 1139
/* 1118 */    MCD_OPC_CheckPredicate, 47, 218, 4, // Skip to: 2364
/* 1122 */    MCD_OPC_CheckField, 21, 5, 17, 212, 4, // Skip to: 2364
/* 1128 */    MCD_OPC_CheckField, 6, 5, 0, 206, 4, // Skip to: 2364
/* 1134 */    MCD_OPC_Decode, 135, 4, 133, 2, // Opcode: C_OLT_D64
/* 1139 */    MCD_OPC_FilterValue, 53, 21, 0, // Skip to: 1164
/* 1143 */    MCD_OPC_CheckPredicate, 47, 193, 4, // Skip to: 2364
/* 1147 */    MCD_OPC_CheckField, 21, 5, 17, 187, 4, // Skip to: 2364
/* 1153 */    MCD_OPC_CheckField, 6, 5, 0, 181, 4, // Skip to: 2364
/* 1159 */    MCD_OPC_Decode, 150, 4, 133, 2, // Opcode: C_ULT_D64
/* 1164 */    MCD_OPC_FilterValue, 54, 21, 0, // Skip to: 1189
/* 1168 */    MCD_OPC_CheckPredicate, 47, 168, 4, // Skip to: 2364
/* 1172 */    MCD_OPC_CheckField, 21, 5, 17, 162, 4, // Skip to: 2364
/* 1178 */    MCD_OPC_CheckField, 6, 5, 0, 156, 4, // Skip to: 2364
/* 1184 */    MCD_OPC_Decode, 132, 4, 133, 2, // Opcode: C_OLE_D64
/* 1189 */    MCD_OPC_FilterValue, 55, 21, 0, // Skip to: 1214
/* 1193 */    MCD_OPC_CheckPredicate, 47, 143, 4, // Skip to: 2364
/* 1197 */    MCD_OPC_CheckField, 21, 5, 17, 137, 4, // Skip to: 2364
/* 1203 */    MCD_OPC_CheckField, 6, 5, 0, 131, 4, // Skip to: 2364
/* 1209 */    MCD_OPC_Decode, 147, 4, 133, 2, // Opcode: C_ULE_D64
/* 1214 */    MCD_OPC_FilterValue, 56, 21, 0, // Skip to: 1239
/* 1218 */    MCD_OPC_CheckPredicate, 47, 118, 4, // Skip to: 2364
/* 1222 */    MCD_OPC_CheckField, 21, 5, 17, 112, 4, // Skip to: 2364
/* 1228 */    MCD_OPC_CheckField, 6, 5, 0, 106, 4, // Skip to: 2364
/* 1234 */    MCD_OPC_Decode, 141, 4, 133, 2, // Opcode: C_SF_D64
/* 1239 */    MCD_OPC_FilterValue, 57, 21, 0, // Skip to: 1264
/* 1243 */    MCD_OPC_CheckPredicate, 47, 93, 4, // Skip to: 2364
/* 1247 */    MCD_OPC_CheckField, 21, 5, 17, 87, 4, // Skip to: 2364
/* 1253 */    MCD_OPC_CheckField, 6, 5, 0, 81, 4, // Skip to: 2364
/* 1259 */    MCD_OPC_Decode, 251, 3, 133, 2, // Opcode: C_NGLE_D64
/* 1264 */    MCD_OPC_FilterValue, 58, 21, 0, // Skip to: 1289
/* 1268 */    MCD_OPC_CheckPredicate, 47, 68, 4, // Skip to: 2364
/* 1272 */    MCD_OPC_CheckField, 21, 5, 17, 62, 4, // Skip to: 2364
/* 1278 */    MCD_OPC_CheckField, 6, 5, 0, 56, 4, // Skip to: 2364
/* 1284 */    MCD_OPC_Decode, 138, 4, 133, 2, // Opcode: C_SEQ_D64
/* 1289 */    MCD_OPC_FilterValue, 59, 21, 0, // Skip to: 1314
/* 1293 */    MCD_OPC_CheckPredicate, 47, 43, 4, // Skip to: 2364
/* 1297 */    MCD_OPC_CheckField, 21, 5, 17, 37, 4, // Skip to: 2364
/* 1303 */    MCD_OPC_CheckField, 6, 5, 0, 31, 4, // Skip to: 2364
/* 1309 */    MCD_OPC_Decode, 254, 3, 133, 2, // Opcode: C_NGL_D64
/* 1314 */    MCD_OPC_FilterValue, 60, 21, 0, // Skip to: 1339
/* 1318 */    MCD_OPC_CheckPredicate, 47, 18, 4, // Skip to: 2364
/* 1322 */    MCD_OPC_CheckField, 21, 5, 17, 12, 4, // Skip to: 2364
/* 1328 */    MCD_OPC_CheckField, 6, 5, 0, 6, 4, // Skip to: 2364
/* 1334 */    MCD_OPC_Decode, 245, 3, 133, 2, // Opcode: C_LT_D64
/* 1339 */    MCD_OPC_FilterValue, 61, 21, 0, // Skip to: 1364
/* 1343 */    MCD_OPC_CheckPredicate, 47, 249, 3, // Skip to: 2364
/* 1347 */    MCD_OPC_CheckField, 21, 5, 17, 243, 3, // Skip to: 2364
/* 1353 */    MCD_OPC_CheckField, 6, 5, 0, 237, 3, // Skip to: 2364
/* 1359 */    MCD_OPC_Decode, 248, 3, 133, 2, // Opcode: C_NGE_D64
/* 1364 */    MCD_OPC_FilterValue, 62, 21, 0, // Skip to: 1389
/* 1368 */    MCD_OPC_CheckPredicate, 47, 224, 3, // Skip to: 2364
/* 1372 */    MCD_OPC_CheckField, 21, 5, 17, 218, 3, // Skip to: 2364
/* 1378 */    MCD_OPC_CheckField, 6, 5, 0, 212, 3, // Skip to: 2364
/* 1384 */    MCD_OPC_Decode, 242, 3, 133, 2, // Opcode: C_LE_D64
/* 1389 */    MCD_OPC_FilterValue, 63, 203, 3, // Skip to: 2364
/* 1393 */    MCD_OPC_CheckPredicate, 47, 199, 3, // Skip to: 2364
/* 1397 */    MCD_OPC_CheckField, 21, 5, 17, 193, 3, // Skip to: 2364
/* 1403 */    MCD_OPC_CheckField, 6, 5, 0, 187, 3, // Skip to: 2364
/* 1409 */    MCD_OPC_Decode, 129, 4, 133, 2, // Opcode: C_NGT_D64
/* 1414 */    MCD_OPC_FilterValue, 18, 41, 0, // Skip to: 1459
/* 1418 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 1421 */    MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 1440
/* 1425 */    MCD_OPC_CheckPredicate, 42, 167, 3, // Skip to: 2364
/* 1429 */    MCD_OPC_CheckField, 3, 8, 0, 161, 3, // Skip to: 2364
/* 1435 */    MCD_OPC_Decode, 198, 4, 255, 1, // Opcode: DMFC2
/* 1440 */    MCD_OPC_FilterValue, 5, 152, 3, // Skip to: 2364
/* 1444 */    MCD_OPC_CheckPredicate, 42, 148, 3, // Skip to: 2364
/* 1448 */    MCD_OPC_CheckField, 3, 8, 0, 142, 3, // Skip to: 2364
/* 1454 */    MCD_OPC_Decode, 203, 4, 255, 1, // Opcode: DMTC2
/* 1459 */    MCD_OPC_FilterValue, 19, 131, 0, // Skip to: 1594
/* 1463 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 1466 */    MCD_OPC_FilterValue, 1, 15, 0, // Skip to: 1485
/* 1470 */    MCD_OPC_CheckPredicate, 48, 122, 3, // Skip to: 2364
/* 1474 */    MCD_OPC_CheckField, 11, 5, 0, 116, 3, // Skip to: 2364
/* 1480 */    MCD_OPC_Decode, 176, 7, 134, 2, // Opcode: LDXC164
/* 1485 */    MCD_OPC_FilterValue, 5, 15, 0, // Skip to: 1504
/* 1489 */    MCD_OPC_CheckPredicate, 49, 103, 3, // Skip to: 2364
/* 1493 */    MCD_OPC_CheckField, 11, 5, 0, 97, 3, // Skip to: 2364
/* 1499 */    MCD_OPC_Decode, 208, 7, 134, 2, // Opcode: LUXC164
/* 1504 */    MCD_OPC_FilterValue, 9, 15, 0, // Skip to: 1523
/* 1508 */    MCD_OPC_CheckPredicate, 48, 84, 3, // Skip to: 2364
/* 1512 */    MCD_OPC_CheckField, 6, 5, 0, 78, 3, // Skip to: 2364
/* 1518 */    MCD_OPC_Decode, 167, 11, 135, 2, // Opcode: SDXC164
/* 1523 */    MCD_OPC_FilterValue, 13, 15, 0, // Skip to: 1542
/* 1527 */    MCD_OPC_CheckPredicate, 49, 65, 3, // Skip to: 2364
/* 1531 */    MCD_OPC_CheckField, 6, 5, 0, 59, 3, // Skip to: 2364
/* 1537 */    MCD_OPC_Decode, 233, 12, 135, 2, // Opcode: SUXC164
/* 1542 */    MCD_OPC_FilterValue, 33, 9, 0, // Skip to: 1555
/* 1546 */    MCD_OPC_CheckPredicate, 48, 46, 3, // Skip to: 2364
/* 1550 */    MCD_OPC_Decode, 144, 8, 136, 2, // Opcode: MADD_D64
/* 1555 */    MCD_OPC_FilterValue, 41, 9, 0, // Skip to: 1568
/* 1559 */    MCD_OPC_CheckPredicate, 48, 33, 3, // Skip to: 2364
/* 1563 */    MCD_OPC_Decode, 162, 9, 136, 2, // Opcode: MSUB_D64
/* 1568 */    MCD_OPC_FilterValue, 49, 9, 0, // Skip to: 1581
/* 1572 */    MCD_OPC_CheckPredicate, 48, 20, 3, // Skip to: 2364
/* 1576 */    MCD_OPC_Decode, 241, 9, 136, 2, // Opcode: NMADD_D64
/* 1581 */    MCD_OPC_FilterValue, 57, 11, 3, // Skip to: 2364
/* 1585 */    MCD_OPC_CheckPredicate, 48, 7, 3, // Skip to: 2364
/* 1589 */    MCD_OPC_Decode, 246, 9, 136, 2, // Opcode: NMSUB_D64
/* 1594 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 1607
/* 1598 */    MCD_OPC_CheckPredicate, 41, 250, 2, // Skip to: 2364
/* 1602 */    MCD_OPC_Decode, 160, 4, 137, 2, // Opcode: DADDi
/* 1607 */    MCD_OPC_FilterValue, 25, 9, 0, // Skip to: 1620
/* 1611 */    MCD_OPC_CheckPredicate, 19, 237, 2, // Skip to: 2364
/* 1615 */    MCD_OPC_Decode, 161, 4, 137, 2, // Opcode: DADDiu
/* 1620 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 1633
/* 1624 */    MCD_OPC_CheckPredicate, 41, 224, 2, // Skip to: 2364
/* 1628 */    MCD_OPC_Decode, 172, 7, 217, 1, // Opcode: LDL
/* 1633 */    MCD_OPC_FilterValue, 27, 9, 0, // Skip to: 1646
/* 1637 */    MCD_OPC_CheckPredicate, 41, 211, 2, // Skip to: 2364
/* 1641 */    MCD_OPC_Decode, 174, 7, 217, 1, // Opcode: LDR
/* 1646 */    MCD_OPC_FilterValue, 28, 159, 1, // Skip to: 2065
/* 1650 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 1653 */    MCD_OPC_FilterValue, 3, 15, 0, // Skip to: 1672
/* 1657 */    MCD_OPC_CheckPredicate, 50, 191, 2, // Skip to: 2364
/* 1661 */    MCD_OPC_CheckField, 6, 5, 0, 185, 2, // Skip to: 2364
/* 1667 */    MCD_OPC_Decode, 206, 4, 224, 1, // Opcode: DMUL
/* 1672 */    MCD_OPC_FilterValue, 8, 15, 0, // Skip to: 1691
/* 1676 */    MCD_OPC_CheckPredicate, 50, 172, 2, // Skip to: 2364
/* 1680 */    MCD_OPC_CheckField, 6, 15, 0, 166, 2, // Skip to: 2364
/* 1686 */    MCD_OPC_Decode, 185, 9, 138, 2, // Opcode: MTM0
/* 1691 */    MCD_OPC_FilterValue, 9, 15, 0, // Skip to: 1710
/* 1695 */    MCD_OPC_CheckPredicate, 50, 153, 2, // Skip to: 2364
/* 1699 */    MCD_OPC_CheckField, 6, 15, 0, 147, 2, // Skip to: 2364
/* 1705 */    MCD_OPC_Decode, 188, 9, 138, 2, // Opcode: MTP0
/* 1710 */    MCD_OPC_FilterValue, 10, 15, 0, // Skip to: 1729
/* 1714 */    MCD_OPC_CheckPredicate, 50, 134, 2, // Skip to: 2364
/* 1718 */    MCD_OPC_CheckField, 6, 15, 0, 128, 2, // Skip to: 2364
/* 1724 */    MCD_OPC_Decode, 189, 9, 138, 2, // Opcode: MTP1
/* 1729 */    MCD_OPC_FilterValue, 11, 15, 0, // Skip to: 1748
/* 1733 */    MCD_OPC_CheckPredicate, 50, 115, 2, // Skip to: 2364
/* 1737 */    MCD_OPC_CheckField, 6, 15, 0, 109, 2, // Skip to: 2364
/* 1743 */    MCD_OPC_Decode, 190, 9, 138, 2, // Opcode: MTP2
/* 1748 */    MCD_OPC_FilterValue, 12, 15, 0, // Skip to: 1767
/* 1752 */    MCD_OPC_CheckPredicate, 50, 96, 2, // Skip to: 2364
/* 1756 */    MCD_OPC_CheckField, 6, 15, 0, 90, 2, // Skip to: 2364
/* 1762 */    MCD_OPC_Decode, 186, 9, 138, 2, // Opcode: MTM1
/* 1767 */    MCD_OPC_FilterValue, 13, 15, 0, // Skip to: 1786
/* 1771 */    MCD_OPC_CheckPredicate, 50, 77, 2, // Skip to: 2364
/* 1775 */    MCD_OPC_CheckField, 6, 15, 0, 71, 2, // Skip to: 2364
/* 1781 */    MCD_OPC_Decode, 187, 9, 138, 2, // Opcode: MTM2
/* 1786 */    MCD_OPC_FilterValue, 15, 15, 0, // Skip to: 1805
/* 1790 */    MCD_OPC_CheckPredicate, 50, 58, 2, // Skip to: 2364
/* 1794 */    MCD_OPC_CheckField, 6, 5, 0, 52, 2, // Skip to: 2364
/* 1800 */    MCD_OPC_Decode, 226, 13, 224, 1, // Opcode: VMULU
/* 1805 */    MCD_OPC_FilterValue, 16, 15, 0, // Skip to: 1824
/* 1809 */    MCD_OPC_CheckPredicate, 50, 39, 2, // Skip to: 2364
/* 1813 */    MCD_OPC_CheckField, 6, 5, 0, 33, 2, // Skip to: 2364
/* 1819 */    MCD_OPC_Decode, 225, 13, 224, 1, // Opcode: VMM0
/* 1824 */    MCD_OPC_FilterValue, 17, 15, 0, // Skip to: 1843
/* 1828 */    MCD_OPC_CheckPredicate, 50, 20, 2, // Skip to: 2364
/* 1832 */    MCD_OPC_CheckField, 6, 5, 0, 14, 2, // Skip to: 2364
/* 1838 */    MCD_OPC_Decode, 224, 13, 224, 1, // Opcode: V3MULU
/* 1843 */    MCD_OPC_FilterValue, 36, 15, 0, // Skip to: 1862
/* 1847 */    MCD_OPC_CheckPredicate, 51, 1, 2, // Skip to: 2364
/* 1851 */    MCD_OPC_CheckField, 6, 5, 0, 251, 1, // Skip to: 2364
/* 1857 */    MCD_OPC_Decode, 170, 4, 139, 2, // Opcode: DCLZ
/* 1862 */    MCD_OPC_FilterValue, 37, 15, 0, // Skip to: 1881
/* 1866 */    MCD_OPC_CheckPredicate, 51, 238, 1, // Skip to: 2364
/* 1870 */    MCD_OPC_CheckField, 6, 5, 0, 232, 1, // Skip to: 2364
/* 1876 */    MCD_OPC_Decode, 168, 4, 139, 2, // Opcode: DCLO
/* 1881 */    MCD_OPC_FilterValue, 40, 15, 0, // Skip to: 1900
/* 1885 */    MCD_OPC_CheckPredicate, 50, 219, 1, // Skip to: 2364
/* 1889 */    MCD_OPC_CheckField, 6, 5, 0, 213, 1, // Skip to: 2364
/* 1895 */    MCD_OPC_Decode, 166, 1, 224, 1, // Opcode: BADDu
/* 1900 */    MCD_OPC_FilterValue, 42, 15, 0, // Skip to: 1919
/* 1904 */    MCD_OPC_CheckPredicate, 50, 200, 1, // Skip to: 2364
/* 1908 */    MCD_OPC_CheckField, 6, 5, 0, 194, 1, // Skip to: 2364
/* 1914 */    MCD_OPC_Decode, 184, 11, 224, 1, // Opcode: SEQ
/* 1919 */    MCD_OPC_FilterValue, 43, 15, 0, // Skip to: 1938
/* 1923 */    MCD_OPC_CheckPredicate, 50, 181, 1, // Skip to: 2364
/* 1927 */    MCD_OPC_CheckField, 6, 5, 0, 175, 1, // Skip to: 2364
/* 1933 */    MCD_OPC_Decode, 252, 11, 224, 1, // Opcode: SNE
/* 1938 */    MCD_OPC_FilterValue, 44, 20, 0, // Skip to: 1962
/* 1942 */    MCD_OPC_CheckPredicate, 50, 162, 1, // Skip to: 2364
/* 1946 */    MCD_OPC_CheckField, 16, 5, 0, 156, 1, // Skip to: 2364
/* 1952 */    MCD_OPC_CheckField, 6, 5, 0, 150, 1, // Skip to: 2364
/* 1958 */    MCD_OPC_Decode, 163, 10, 62, // Opcode: POP
/* 1962 */    MCD_OPC_FilterValue, 45, 21, 0, // Skip to: 1987
/* 1966 */    MCD_OPC_CheckPredicate, 50, 138, 1, // Skip to: 2364
/* 1970 */    MCD_OPC_CheckField, 16, 5, 0, 132, 1, // Skip to: 2364
/* 1976 */    MCD_OPC_CheckField, 6, 5, 0, 126, 1, // Skip to: 2364
/* 1982 */    MCD_OPC_Decode, 231, 4, 222, 1, // Opcode: DPOP
/* 1987 */    MCD_OPC_FilterValue, 46, 9, 0, // Skip to: 2000
/* 1991 */    MCD_OPC_CheckPredicate, 50, 113, 1, // Skip to: 2364
/* 1995 */    MCD_OPC_Decode, 185, 11, 140, 2, // Opcode: SEQi
/* 2000 */    MCD_OPC_FilterValue, 47, 9, 0, // Skip to: 2013
/* 2004 */    MCD_OPC_CheckPredicate, 50, 100, 1, // Skip to: 2364
/* 2008 */    MCD_OPC_Decode, 253, 11, 140, 2, // Opcode: SNEi
/* 2013 */    MCD_OPC_FilterValue, 50, 9, 0, // Skip to: 2026
/* 2017 */    MCD_OPC_CheckPredicate, 50, 87, 1, // Skip to: 2364
/* 2021 */    MCD_OPC_Decode, 241, 2, 141, 2, // Opcode: CINS
/* 2026 */    MCD_OPC_FilterValue, 51, 9, 0, // Skip to: 2039
/* 2030 */    MCD_OPC_CheckPredicate, 50, 74, 1, // Skip to: 2364
/* 2034 */    MCD_OPC_Decode, 242, 2, 141, 2, // Opcode: CINS32
/* 2039 */    MCD_OPC_FilterValue, 58, 9, 0, // Skip to: 2052
/* 2043 */    MCD_OPC_CheckPredicate, 50, 61, 1, // Skip to: 2364
/* 2047 */    MCD_OPC_Decode, 158, 5, 141, 2, // Opcode: EXTS
/* 2052 */    MCD_OPC_FilterValue, 59, 52, 1, // Skip to: 2364
/* 2056 */    MCD_OPC_CheckPredicate, 50, 48, 1, // Skip to: 2364
/* 2060 */    MCD_OPC_Decode, 159, 5, 141, 2, // Opcode: EXTS32
/* 2065 */    MCD_OPC_FilterValue, 31, 126, 0, // Skip to: 2195
/* 2069 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 2072 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 2085
/* 2076 */    MCD_OPC_CheckPredicate, 6, 28, 1, // Skip to: 2364
/* 2080 */    MCD_OPC_Decode, 177, 4, 142, 2, // Opcode: DEXTM
/* 2085 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 2098
/* 2089 */    MCD_OPC_CheckPredicate, 6, 15, 1, // Skip to: 2364
/* 2093 */    MCD_OPC_Decode, 178, 4, 142, 2, // Opcode: DEXTU
/* 2098 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 2111
/* 2102 */    MCD_OPC_CheckPredicate, 6, 2, 1, // Skip to: 2364
/* 2106 */    MCD_OPC_Decode, 176, 4, 142, 2, // Opcode: DEXT
/* 2111 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 2124
/* 2115 */    MCD_OPC_CheckPredicate, 6, 245, 0, // Skip to: 2364
/* 2119 */    MCD_OPC_Decode, 181, 4, 143, 2, // Opcode: DINSM
/* 2124 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 2137
/* 2128 */    MCD_OPC_CheckPredicate, 6, 232, 0, // Skip to: 2364
/* 2132 */    MCD_OPC_Decode, 182, 4, 143, 2, // Opcode: DINSU
/* 2137 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 2150
/* 2141 */    MCD_OPC_CheckPredicate, 6, 219, 0, // Skip to: 2364
/* 2145 */    MCD_OPC_Decode, 180, 4, 143, 2, // Opcode: DINS
/* 2150 */    MCD_OPC_FilterValue, 36, 210, 0, // Skip to: 2364
/* 2154 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 2157 */    MCD_OPC_FilterValue, 2, 15, 0, // Skip to: 2176
/* 2161 */    MCD_OPC_CheckPredicate, 40, 199, 0, // Skip to: 2364
/* 2165 */    MCD_OPC_CheckField, 21, 5, 0, 193, 0, // Skip to: 2364
/* 2171 */    MCD_OPC_Decode, 249, 4, 243, 1, // Opcode: DSBH
/* 2176 */    MCD_OPC_FilterValue, 5, 184, 0, // Skip to: 2364
/* 2180 */    MCD_OPC_CheckPredicate, 40, 180, 0, // Skip to: 2364
/* 2184 */    MCD_OPC_CheckField, 21, 5, 0, 174, 0, // Skip to: 2364
/* 2190 */    MCD_OPC_Decode, 251, 4, 243, 1, // Opcode: DSHD
/* 2195 */    MCD_OPC_FilterValue, 39, 9, 0, // Skip to: 2208
/* 2199 */    MCD_OPC_CheckPredicate, 19, 161, 0, // Skip to: 2364
/* 2203 */    MCD_OPC_Decode, 241, 7, 217, 1, // Opcode: LWu
/* 2208 */    MCD_OPC_FilterValue, 44, 9, 0, // Skip to: 2221
/* 2212 */    MCD_OPC_CheckPredicate, 41, 148, 0, // Skip to: 2364
/* 2216 */    MCD_OPC_Decode, 164, 11, 217, 1, // Opcode: SDL
/* 2221 */    MCD_OPC_FilterValue, 45, 9, 0, // Skip to: 2234
/* 2225 */    MCD_OPC_CheckPredicate, 41, 135, 0, // Skip to: 2364
/* 2229 */    MCD_OPC_Decode, 165, 11, 217, 1, // Opcode: SDR
/* 2234 */    MCD_OPC_FilterValue, 50, 9, 0, // Skip to: 2247
/* 2238 */    MCD_OPC_CheckPredicate, 50, 122, 0, // Skip to: 2364
/* 2242 */    MCD_OPC_Decode, 171, 1, 144, 2, // Opcode: BBIT0
/* 2247 */    MCD_OPC_FilterValue, 52, 9, 0, // Skip to: 2260
/* 2251 */    MCD_OPC_CheckPredicate, 41, 109, 0, // Skip to: 2364
/* 2255 */    MCD_OPC_Decode, 194, 7, 217, 1, // Opcode: LLD
/* 2260 */    MCD_OPC_FilterValue, 53, 9, 0, // Skip to: 2273
/* 2264 */    MCD_OPC_CheckPredicate, 52, 96, 0, // Skip to: 2364
/* 2268 */    MCD_OPC_Decode, 163, 7, 219, 1, // Opcode: LDC164
/* 2273 */    MCD_OPC_FilterValue, 54, 9, 0, // Skip to: 2286
/* 2277 */    MCD_OPC_CheckPredicate, 50, 83, 0, // Skip to: 2364
/* 2281 */    MCD_OPC_Decode, 172, 1, 144, 2, // Opcode: BBIT032
/* 2286 */    MCD_OPC_FilterValue, 55, 9, 0, // Skip to: 2299
/* 2290 */    MCD_OPC_CheckPredicate, 19, 70, 0, // Skip to: 2364
/* 2294 */    MCD_OPC_Decode, 161, 7, 217, 1, // Opcode: LD
/* 2299 */    MCD_OPC_FilterValue, 58, 9, 0, // Skip to: 2312
/* 2303 */    MCD_OPC_CheckPredicate, 50, 57, 0, // Skip to: 2364
/* 2307 */    MCD_OPC_Decode, 173, 1, 144, 2, // Opcode: BBIT1
/* 2312 */    MCD_OPC_FilterValue, 60, 9, 0, // Skip to: 2325
/* 2316 */    MCD_OPC_CheckPredicate, 41, 44, 0, // Skip to: 2364
/* 2320 */    MCD_OPC_Decode, 147, 11, 217, 1, // Opcode: SCD
/* 2325 */    MCD_OPC_FilterValue, 61, 9, 0, // Skip to: 2338
/* 2329 */    MCD_OPC_CheckPredicate, 52, 31, 0, // Skip to: 2364
/* 2333 */    MCD_OPC_Decode, 157, 11, 219, 1, // Opcode: SDC164
/* 2338 */    MCD_OPC_FilterValue, 62, 9, 0, // Skip to: 2351
/* 2342 */    MCD_OPC_CheckPredicate, 50, 18, 0, // Skip to: 2364
/* 2346 */    MCD_OPC_Decode, 174, 1, 144, 2, // Opcode: BBIT132
/* 2351 */    MCD_OPC_FilterValue, 63, 9, 0, // Skip to: 2364
/* 2355 */    MCD_OPC_CheckPredicate, 19, 5, 0, // Skip to: 2364
/* 2359 */    MCD_OPC_Decode, 151, 11, 217, 1, // Opcode: SD
/* 2364 */    MCD_OPC_Fail,
  0
};

static bool getbool(uint64_t b)
{
	return b != 0;
}

static bool checkDecoderPredicate(unsigned Idx, uint64_t Bits)
{
  switch (Idx) {
  default: // llvm_unreachable("Invalid index!");
  case 0:
    return getbool((Bits & Mips_FeatureMips16));
  case 1:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureMicroMips));
  case 2:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2) && !(Bits & Mips_FeatureMicroMips));
  case 3:
    return getbool((Bits & Mips_FeatureMicroMips));
  case 4:
    return getbool((Bits & Mips_FeatureMips32) && (Bits & Mips_FeatureMicroMips));
  case 5:
    return getbool(!(Bits & Mips_FeatureMips16));
  case 6:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32r2));
  case 7:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips4_32) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 8:
    return getbool((Bits & Mips_FeatureMSA));
  case 9:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 10:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32));
  case 11:
    return getbool(!(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureMicroMips));
  case 12:
    return getbool((Bits & Mips_FeatureDSP));
  case 13:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 14:
    return getbool((Bits & Mips_FeatureMSA) && (Bits & Mips_FeatureMips64));
  case 15:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2));
  case 16:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 17:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips3_32));
  case 18:
    return getbool(!(Bits & Mips_FeatureMicroMips));
  case 19:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips3));
  case 20:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32r2) && !(Bits & Mips_FeatureFP64Bit));
  case 21:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit));
  case 22:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips3_32r2));
  case 23:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2) && !(Bits & Mips_FeatureFP64Bit));
  case 24:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips4_32) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 25:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureFP64Bit));
  case 26:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips4_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 27:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips4_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureMicroMips));
  case 28:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips5_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 29:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips4_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 30:
    return getbool((Bits & Mips_FeatureDSPR2));
  case 31:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips3_32) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 32:
    return getbool((Bits & Mips_FeatureMips2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureMicroMips));
  case 33:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureMicroMips));
  case 34:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips2));
  case 35:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && !(Bits & Mips_FeatureMicroMips));
  case 36:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32r6));
  case 37:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips64r6));
  case 38:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureGP64Bit) && (Bits & Mips_FeatureMips32r6));
  case 39:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureGP64Bit) && (Bits & Mips_FeatureMips32r6));
  case 40:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips64r2));
  case 41:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips3) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 42:
    return getbool((Bits & Mips_FeatureMips64));
  case 43:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips32r2) && (Bits & Mips_FeatureFP64Bit));
  case 44:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureFP64Bit));
  case 45:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips2) && (Bits & Mips_FeatureFP64Bit));
  case 46:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips4_32) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 47:
    return getbool(!(Bits & Mips_FeatureMips16) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6) && (Bits & Mips_FeatureFP64Bit));
  case 48:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips4_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 49:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips5_32r2) && !(Bits & Mips_FeatureMips32r6) && !(Bits & Mips_FeatureMips64r6));
  case 50:
    return getbool((Bits & Mips_FeatureCnMips));
  case 51:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureMips64) && !(Bits & Mips_FeatureMips64r6));
  case 52:
    return getbool(!(Bits & Mips_FeatureMips16) && (Bits & Mips_FeatureFP64Bit) && (Bits & Mips_FeatureMips2));
  }
}

#define DecodeToMCInst(fname,fieldname, InsnType) \
static DecodeStatus fname(DecodeStatus S, unsigned Idx, InsnType insn, MCInst *MI, \
                uint64_t Address, void *Decoder) \
{ \
  InsnType tmp; \
  switch (Idx) { \
  default: \
  case 0: \
    return S; \
  case 1: \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 2: \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 3: \
    tmp = 0; \
    tmp |= fieldname(insn, 3, 2) << 3; \
    tmp |= fieldname(insn, 5, 3) << 0; \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 4: \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 5: \
    tmp = fieldname(insn, 2, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 5, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 6: \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 5, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 7: \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 5, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 8: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 5) << 0; \
    tmp |= fieldname(insn, 16, 5) << 11; \
    tmp |= fieldname(insn, 21, 6) << 5; \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 9: \
    tmp = fieldname(insn, 5, 3); \
    if (DecodeCPU16RegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 10: \
    if (DecodeFMem3(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 11: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 12: \
    if (DecodeMemMMImm4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 13: \
    tmp = fieldname(insn, 5, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 14: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 15: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeANDI16Imm(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 16: \
    tmp = fieldname(insn, 3, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 17: \
    tmp = fieldname(insn, 3, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 3, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 18: \
    if (DecodeMemMMReglistImm4Lsl2(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 19: \
    tmp = fieldname(insn, 0, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 20: \
    tmp = fieldname(insn, 0, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 21: \
    tmp = fieldname(insn, 0, 5); \
    if (DecodeUImm5lsl2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 22: \
    if (DecodeMemMMSPImm5Lsl2(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 23: \
    tmp = fieldname(insn, 5, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 5, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 4); \
    if (DecodeSimm4(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 24: \
    tmp = fieldname(insn, 1, 9); \
    if (DecodeSimm9SP(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 25: \
    if (DecodeMemMMGPImm7Lsl2(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 26: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 3); \
    if (DecodeAddiur2Simm7(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 27: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 6); \
    if (DecodeUImm6Lsl2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 28: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeMovePRegPair(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 3); \
    if (DecodeGPRMM16MovePRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 3); \
    if (DecodeGPRMM16MovePRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 29: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 7); \
    if (DecodeBranchTarget7MM(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 30: \
    tmp = fieldname(insn, 0, 10); \
    if (DecodeBranchTarget10MM(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 31: \
    tmp = fieldname(insn, 7, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 7); \
    if (DecodeLiSimm7(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 32: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 33: \
    tmp = fieldname(insn, 16, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 6, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 34: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeInsSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 35: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 36: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 37: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 38: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 39: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeExtSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 40: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 41: \
    tmp = fieldname(insn, 16, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 42: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 43: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 44: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeHWRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 45: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 46: \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 47: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 48: \
    if (DecodeMemMMImm16(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 49: \
    if (DecodeMemMMImm12(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 50: \
    if (DecodeCacheOpMM(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 51: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTargetMM(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 52: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 53: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 54: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 55: \
    if (DecodeJumpTargetMM(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 56: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeGPRMM16RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 23); \
    if (DecodeSimm23Lsl2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 57: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTargetMM(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 58: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 59: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 60: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 2); \
    if (DecodeLSAImm(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 61: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 62: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 63: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 64: \
    tmp = fieldname(insn, 6, 20); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 65: \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 66: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 67: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 68: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeHI32DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 69: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeLO32DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 70: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 2); \
    if (DecodeLSAImm(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 71: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 72: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 73: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 74: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 75: \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 76: \
    if (DecodeSyncI(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 77: \
    if (DecodeJumpTarget(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 78: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 79: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 80: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 81: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 82: \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 83: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 84: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 85: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeCCRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 86: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 87: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 88: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 89: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeCCRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 90: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 91: \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 92: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 93: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 94: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 95: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 96: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 97: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 98: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 99: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 100: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 101: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 102: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 103: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 104: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 105: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 106: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 107: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 108: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 109: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 110: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 111: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 112: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 113: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 114: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 115: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeAFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 116: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 117: \
    tmp = 0; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 118: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 119: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 120: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 121: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 122: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 123: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 124: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 125: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 126: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 127: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 128: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 129: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 130: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 6); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 131: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 132: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 133: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 6); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 134: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 135: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 136: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 137: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 138: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 139: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 140: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 141: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 142: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 143: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 144: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 145: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 146: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 147: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 148: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 149: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 150: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 151: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 152: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 153: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 154: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 155: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 156: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 157: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 158: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 159: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 160: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 161: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 162: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 163: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSACtrlRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 164: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 165: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 166: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 167: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 168: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSACtrlRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 169: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 170: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 171: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 172: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 173: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 174: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 175: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 176: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 177: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 178: \
    if (DecodeINSVE_DF_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 179: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 180: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 181: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128BRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 182: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 183: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 184: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 185: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 186: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 187: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 188: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128HRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 189: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeMSA128DRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeMSA128WRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 190: \
    if (DecodeMSA128Mem(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 191: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeExtSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 192: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeInsSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 193: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 194: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 195: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 196: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 197: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 198: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 199: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 200: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 201: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 202: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 203: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 204: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 205: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 206: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 207: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeDSPRRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 208: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 209: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 210: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 211: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 212: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 213: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 214: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 6); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 215: \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    if (DecodeACC64DSPRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 216: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeHWRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 217: \
    if (DecodeMem(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 218: \
    if (DecodeCacheOp(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 219: \
    if (DecodeFMem(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 220: \
    if (DecodeFMem2(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 221: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 222: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 223: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 224: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 225: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 226: \
    if (DecodeBlezGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 227: \
    if (DecodeBgtzGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 228: \
    if (DecodeAddiGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 229: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 230: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGRCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 231: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 232: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGRCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 233: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 234: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 235: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGRCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 236: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGRCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 237: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeCOP2RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 238: \
    if (DecodeFMemCop2R6(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 239: \
    if (DecodeBlezlGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 240: \
    if (DecodeBgtzlGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 241: \
    if (DecodeDaddiGroupBranch_4(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 242: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 243: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 244: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 245: \
    if (DecodeCacheOpR6(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 246: \
    if (DecodeSpecial3LlSc(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 247: \
    tmp = fieldname(insn, 0, 26); \
    if (DecodeBranchTarget26(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 248: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 21); \
    if (DecodeBranchTarget21(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 249: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 19); \
    if (DecodeSimm19Lsl2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 250: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 18); \
    if (DecodeSimm18Lsl3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 251: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 252: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 253: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 254: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 255: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 256: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 257: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 258: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 259: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeFCCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 260: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR32RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 261: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 262: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 263: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePtrRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 264: \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeFGR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 265: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeSimm16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 266: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 267: \
    tmp = 0; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 268: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 269: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 270: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeExtSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 271: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeInsSize(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 272: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPR64RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 0, 16); \
    if (DecodeBranchTarget(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  } \
}

#define DecodeInstruction(fname, fieldname, decoder, InsnType) \
static DecodeStatus fname(const uint8_t DecodeTable[], MCInst *MI, \
           InsnType insn, uint64_t Address, MCRegisterInfo *MRI, int feature) \
{ \
  uint64_t Bits = getFeatureBits(feature); \
  const uint8_t *Ptr = DecodeTable; \
  uint32_t CurFieldValue = 0, ExpectedValue; \
  DecodeStatus S = MCDisassembler_Success; \
  unsigned Start, Len, NumToSkip, PIdx, Opc, DecodeIdx; \
  InsnType Val, FieldValue, PositiveMask, NegativeMask; \
  bool Pred, Fail; \
  for (;;) { \
    switch (*Ptr) { \
    default: \
      return MCDisassembler_Fail; \
    case MCD_OPC_ExtractField: { \
      Start = *++Ptr; \
      Len = *++Ptr; \
      ++Ptr; \
      CurFieldValue = (uint32_t)fieldname(insn, Start, Len); \
      break; \
    } \
    case MCD_OPC_FilterValue: { \
      Val = (InsnType)decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NumToSkip = *Ptr++; \
      NumToSkip |= (*Ptr++) << 8; \
      if (Val != CurFieldValue) \
        Ptr += NumToSkip; \
      break; \
    } \
    case MCD_OPC_CheckField: { \
      Start = *++Ptr; \
      Len = *++Ptr; \
      FieldValue = fieldname(insn, Start, Len); \
      ExpectedValue = (uint32_t)decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NumToSkip = *Ptr++; \
      NumToSkip |= (*Ptr++) << 8; \
      if (ExpectedValue != FieldValue) \
        Ptr += NumToSkip; \
      break; \
    } \
    case MCD_OPC_CheckPredicate: { \
      PIdx = (uint32_t)decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NumToSkip = *Ptr++; \
      NumToSkip |= (*Ptr++) << 8; \
      Pred = checkDecoderPredicate(PIdx, Bits); \
      if (!Pred) \
        Ptr += NumToSkip; \
      (void)Pred; \
      break; \
    } \
    case MCD_OPC_Decode: { \
      Opc = (unsigned)decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      DecodeIdx = (unsigned)decodeULEB128(Ptr, &Len); \
      Ptr += Len; \
      MCInst_setOpcode(MI, Opc); \
      return decoder(S, DecodeIdx, insn, MI, Address, MRI); \
    } \
    case MCD_OPC_SoftFail: { \
      PositiveMask = (InsnType)decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NegativeMask = (InsnType)decodeULEB128(Ptr, &Len); \
      Ptr += Len; \
      Fail = (insn & PositiveMask) || (~insn & NegativeMask); \
      if (Fail) \
        S = MCDisassembler_SoftFail; \
      break; \
    } \
    case MCD_OPC_Fail: { \
      return MCDisassembler_Fail; \
    } \
    } \
  } \
}

FieldFromInstruction(fieldFromInstruction, uint32_t)
DecodeToMCInst(decodeToMCInst, fieldFromInstruction, uint32_t)
DecodeInstruction(decodeInstruction, fieldFromInstruction, decodeToMCInst, uint32_t)
