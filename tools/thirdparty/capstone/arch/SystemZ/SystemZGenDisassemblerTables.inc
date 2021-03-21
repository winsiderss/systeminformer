/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* * SystemZ Disassembler                                                     *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* Capstone Disassembly Engine */
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

static const uint8_t DecoderTable16[] = {
/* 0 */       MCD_OPC_ExtractField, 8, 8,  // Inst{15-8} ...
/* 3 */       MCD_OPC_FilterValue, 7, 127, 0, // Skip to: 134
/* 7 */       MCD_OPC_ExtractField, 4, 4,  // Inst{7-4} ...
/* 10 */      MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 18
/* 14 */      MCD_OPC_Decode, 244, 2, 0, // Opcode: AsmOBR
/* 18 */      MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 26
/* 22 */      MCD_OPC_Decode, 164, 1, 0, // Opcode: AsmHBR
/* 26 */      MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 34
/* 30 */      MCD_OPC_Decode, 209, 2, 0, // Opcode: AsmNLEBR
/* 34 */      MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 42
/* 38 */      MCD_OPC_Decode, 150, 2, 0, // Opcode: AsmLBR
/* 42 */      MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 50
/* 46 */      MCD_OPC_Decode, 191, 2, 0, // Opcode: AsmNHEBR
/* 50 */      MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 58
/* 54 */      MCD_OPC_Decode, 160, 2, 0, // Opcode: AsmLHBR
/* 58 */      MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 66
/* 62 */      MCD_OPC_Decode, 181, 2, 0, // Opcode: AsmNEBR
/* 66 */      MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 74
/* 70 */      MCD_OPC_Decode, 155, 1, 0, // Opcode: AsmEBR
/* 74 */      MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 82
/* 78 */      MCD_OPC_Decode, 218, 2, 0, // Opcode: AsmNLHBR
/* 82 */      MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 90
/* 86 */      MCD_OPC_Decode, 165, 1, 0, // Opcode: AsmHEBR
/* 90 */      MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 98
/* 94 */      MCD_OPC_Decode, 208, 2, 0, // Opcode: AsmNLBR
/* 98 */      MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 106
/* 102 */     MCD_OPC_Decode, 151, 2, 0, // Opcode: AsmLEBR
/* 106 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 114
/* 110 */     MCD_OPC_Decode, 190, 2, 0, // Opcode: AsmNHBR
/* 114 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 122
/* 118 */     MCD_OPC_Decode, 235, 2, 0, // Opcode: AsmNOBR
/* 122 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 130
/* 126 */     MCD_OPC_Decode, 128, 3, 0, // Opcode: BR
/* 130 */     MCD_OPC_Decode, 144, 1, 1, // Opcode: AsmBCR
/* 134 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 142
/* 138 */     MCD_OPC_Decode, 255, 2, 2, // Opcode: BASR
/* 142 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 150
/* 146 */     MCD_OPC_Decode, 145, 5, 3, // Opcode: LPR
/* 150 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 158
/* 154 */     MCD_OPC_Decode, 135, 5, 3, // Opcode: LNR
/* 158 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 166
/* 162 */     MCD_OPC_Decode, 163, 5, 3, // Opcode: LTR
/* 166 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 174
/* 170 */     MCD_OPC_Decode, 192, 4, 3, // Opcode: LCR
/* 174 */     MCD_OPC_FilterValue, 20, 4, 0, // Skip to: 182
/* 178 */     MCD_OPC_Decode, 242, 5, 4, // Opcode: NR
/* 182 */     MCD_OPC_FilterValue, 21, 4, 0, // Skip to: 190
/* 186 */     MCD_OPC_Decode, 210, 3, 3, // Opcode: CLR
/* 190 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 198
/* 194 */     MCD_OPC_Decode, 141, 6, 4, // Opcode: OR
/* 198 */     MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 206
/* 202 */     MCD_OPC_Decode, 142, 7, 4, // Opcode: XR
/* 206 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 214
/* 210 */     MCD_OPC_Decode, 147, 5, 3, // Opcode: LR
/* 214 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 222
/* 218 */     MCD_OPC_Decode, 221, 3, 3, // Opcode: CR
/* 222 */     MCD_OPC_FilterValue, 26, 3, 0, // Skip to: 229
/* 226 */     MCD_OPC_Decode, 66, 4, // Opcode: AR
/* 229 */     MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 237
/* 233 */     MCD_OPC_Decode, 196, 6, 4, // Opcode: SR
/* 237 */     MCD_OPC_FilterValue, 30, 3, 0, // Skip to: 244
/* 241 */     MCD_OPC_Decode, 63, 4, // Opcode: ALR
/* 244 */     MCD_OPC_FilterValue, 31, 4, 0, // Skip to: 252
/* 248 */     MCD_OPC_Decode, 188, 6, 4, // Opcode: SLR
/* 252 */     MCD_OPC_FilterValue, 40, 4, 0, // Skip to: 260
/* 256 */     MCD_OPC_Decode, 198, 4, 5, // Opcode: LDR
/* 260 */     MCD_OPC_FilterValue, 56, 4, 0, // Skip to: 268
/* 264 */     MCD_OPC_Decode, 205, 4, 6, // Opcode: LER
/* 268 */     MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTable32[] = {
/* 0 */       MCD_OPC_ExtractField, 24, 8,  // Inst{31-24} ...
/* 3 */       MCD_OPC_FilterValue, 64, 4, 0, // Skip to: 11
/* 7 */       MCD_OPC_Decode, 219, 6, 7, // Opcode: STH
/* 11 */      MCD_OPC_FilterValue, 65, 4, 0, // Skip to: 19
/* 15 */      MCD_OPC_Decode, 171, 4, 8, // Opcode: LA
/* 19 */      MCD_OPC_FilterValue, 66, 4, 0, // Skip to: 27
/* 23 */      MCD_OPC_Decode, 208, 6, 7, // Opcode: STC
/* 27 */      MCD_OPC_FilterValue, 67, 4, 0, // Skip to: 35
/* 31 */      MCD_OPC_Decode, 147, 4, 9, // Opcode: IC
/* 35 */      MCD_OPC_FilterValue, 72, 4, 0, // Skip to: 43
/* 39 */      MCD_OPC_Decode, 224, 4, 7, // Opcode: LH
/* 43 */      MCD_OPC_FilterValue, 73, 4, 0, // Skip to: 51
/* 47 */      MCD_OPC_Decode, 169, 3, 7, // Opcode: CH
/* 51 */      MCD_OPC_FilterValue, 74, 3, 0, // Skip to: 58
/* 55 */      MCD_OPC_Decode, 42, 10, // Opcode: AH
/* 58 */      MCD_OPC_FilterValue, 75, 4, 0, // Skip to: 66
/* 62 */      MCD_OPC_Decode, 171, 6, 10, // Opcode: SH
/* 66 */      MCD_OPC_FilterValue, 76, 4, 0, // Skip to: 74
/* 70 */      MCD_OPC_Decode, 187, 5, 10, // Opcode: MH
/* 74 */      MCD_OPC_FilterValue, 80, 4, 0, // Skip to: 82
/* 78 */      MCD_OPC_Decode, 206, 6, 7, // Opcode: ST
/* 82 */      MCD_OPC_FilterValue, 84, 4, 0, // Skip to: 90
/* 86 */      MCD_OPC_Decode, 218, 5, 10, // Opcode: N
/* 90 */      MCD_OPC_FilterValue, 85, 4, 0, // Skip to: 98
/* 94 */      MCD_OPC_Decode, 178, 3, 7, // Opcode: CL
/* 98 */      MCD_OPC_FilterValue, 86, 4, 0, // Skip to: 106
/* 102 */     MCD_OPC_Decode, 245, 5, 10, // Opcode: O
/* 106 */     MCD_OPC_FilterValue, 87, 4, 0, // Skip to: 114
/* 110 */     MCD_OPC_Decode, 128, 7, 10, // Opcode: X
/* 114 */     MCD_OPC_FilterValue, 88, 4, 0, // Skip to: 122
/* 118 */     MCD_OPC_Decode, 169, 4, 7, // Opcode: L
/* 122 */     MCD_OPC_FilterValue, 89, 4, 0, // Skip to: 130
/* 126 */     MCD_OPC_Decode, 135, 3, 7, // Opcode: C
/* 130 */     MCD_OPC_FilterValue, 90, 3, 0, // Skip to: 137
/* 134 */     MCD_OPC_Decode, 22, 10, // Opcode: A
/* 137 */     MCD_OPC_FilterValue, 91, 4, 0, // Skip to: 145
/* 141 */     MCD_OPC_Decode, 161, 6, 10, // Opcode: S
/* 145 */     MCD_OPC_FilterValue, 94, 3, 0, // Skip to: 152
/* 149 */     MCD_OPC_Decode, 49, 10, // Opcode: AL
/* 152 */     MCD_OPC_FilterValue, 95, 4, 0, // Skip to: 160
/* 156 */     MCD_OPC_Decode, 173, 6, 10, // Opcode: SL
/* 160 */     MCD_OPC_FilterValue, 96, 4, 0, // Skip to: 168
/* 164 */     MCD_OPC_Decode, 212, 6, 11, // Opcode: STD
/* 168 */     MCD_OPC_FilterValue, 104, 4, 0, // Skip to: 176
/* 172 */     MCD_OPC_Decode, 194, 4, 11, // Opcode: LD
/* 176 */     MCD_OPC_FilterValue, 112, 4, 0, // Skip to: 184
/* 180 */     MCD_OPC_Decode, 214, 6, 12, // Opcode: STE
/* 184 */     MCD_OPC_FilterValue, 113, 4, 0, // Skip to: 192
/* 188 */     MCD_OPC_Decode, 192, 5, 10, // Opcode: MS
/* 192 */     MCD_OPC_FilterValue, 120, 4, 0, // Skip to: 200
/* 196 */     MCD_OPC_Decode, 202, 4, 12, // Opcode: LE
/* 200 */     MCD_OPC_FilterValue, 136, 1, 10, 0, // Skip to: 215
/* 205 */     MCD_OPC_CheckField, 16, 4, 0, 218, 11, // Skip to: 3245
/* 211 */     MCD_OPC_Decode, 201, 6, 13, // Opcode: SRL
/* 215 */     MCD_OPC_FilterValue, 137, 1, 10, 0, // Skip to: 230
/* 220 */     MCD_OPC_CheckField, 16, 4, 0, 203, 11, // Skip to: 3245
/* 226 */     MCD_OPC_Decode, 185, 6, 13, // Opcode: SLL
/* 230 */     MCD_OPC_FilterValue, 138, 1, 10, 0, // Skip to: 245
/* 235 */     MCD_OPC_CheckField, 16, 4, 0, 188, 11, // Skip to: 3245
/* 241 */     MCD_OPC_Decode, 197, 6, 13, // Opcode: SRA
/* 245 */     MCD_OPC_FilterValue, 145, 1, 4, 0, // Skip to: 254
/* 250 */     MCD_OPC_Decode, 244, 6, 14, // Opcode: TM
/* 254 */     MCD_OPC_FilterValue, 146, 1, 4, 0, // Skip to: 263
/* 259 */     MCD_OPC_Decode, 211, 5, 14, // Opcode: MVI
/* 263 */     MCD_OPC_FilterValue, 148, 1, 4, 0, // Skip to: 272
/* 268 */     MCD_OPC_Decode, 225, 5, 14, // Opcode: NI
/* 272 */     MCD_OPC_FilterValue, 149, 1, 4, 0, // Skip to: 281
/* 277 */     MCD_OPC_Decode, 205, 3, 14, // Opcode: CLI
/* 281 */     MCD_OPC_FilterValue, 150, 1, 4, 0, // Skip to: 290
/* 286 */     MCD_OPC_Decode, 252, 5, 14, // Opcode: OI
/* 290 */     MCD_OPC_FilterValue, 151, 1, 4, 0, // Skip to: 299
/* 295 */     MCD_OPC_Decode, 135, 7, 14, // Opcode: XI
/* 299 */     MCD_OPC_FilterValue, 165, 1, 131, 0, // Skip to: 435
/* 304 */     MCD_OPC_ExtractField, 16, 4,  // Inst{19-16} ...
/* 307 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 315
/* 311 */     MCD_OPC_Decode, 154, 4, 15, // Opcode: IIHH
/* 315 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 323
/* 319 */     MCD_OPC_Decode, 156, 4, 15, // Opcode: IIHL
/* 323 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 331
/* 327 */     MCD_OPC_Decode, 161, 4, 16, // Opcode: IILH
/* 331 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 339
/* 335 */     MCD_OPC_Decode, 163, 4, 16, // Opcode: IILL
/* 339 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 347
/* 343 */     MCD_OPC_Decode, 229, 5, 15, // Opcode: NIHH
/* 347 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 355
/* 351 */     MCD_OPC_Decode, 231, 5, 15, // Opcode: NIHL
/* 355 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 363
/* 359 */     MCD_OPC_Decode, 236, 5, 16, // Opcode: NILH
/* 363 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 371
/* 367 */     MCD_OPC_Decode, 238, 5, 16, // Opcode: NILL
/* 371 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 379
/* 375 */     MCD_OPC_Decode, 128, 6, 15, // Opcode: OIHH
/* 379 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 387
/* 383 */     MCD_OPC_Decode, 130, 6, 15, // Opcode: OIHL
/* 387 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 395
/* 391 */     MCD_OPC_Decode, 135, 6, 16, // Opcode: OILH
/* 395 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 403
/* 399 */     MCD_OPC_Decode, 137, 6, 16, // Opcode: OILL
/* 403 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 411
/* 407 */     MCD_OPC_Decode, 252, 4, 17, // Opcode: LLIHH
/* 411 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 419
/* 415 */     MCD_OPC_Decode, 253, 4, 17, // Opcode: LLIHL
/* 419 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 427
/* 423 */     MCD_OPC_Decode, 255, 4, 17, // Opcode: LLILH
/* 427 */     MCD_OPC_FilterValue, 15, 254, 10, // Skip to: 3245
/* 431 */     MCD_OPC_Decode, 128, 5, 17, // Opcode: LLILL
/* 435 */     MCD_OPC_FilterValue, 167, 1, 252, 0, // Skip to: 692
/* 440 */     MCD_OPC_ExtractField, 16, 4,  // Inst{19-16} ...
/* 443 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 451
/* 447 */     MCD_OPC_Decode, 250, 6, 18, // Opcode: TMLH
/* 451 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 459
/* 455 */     MCD_OPC_Decode, 252, 6, 18, // Opcode: TMLL
/* 459 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 467
/* 463 */     MCD_OPC_Decode, 245, 6, 19, // Opcode: TMHH
/* 467 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 475
/* 471 */     MCD_OPC_Decode, 247, 6, 19, // Opcode: TMHL
/* 475 */     MCD_OPC_FilterValue, 4, 127, 0, // Skip to: 606
/* 479 */     MCD_OPC_ExtractField, 20, 4,  // Inst{23-20} ...
/* 482 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 490
/* 486 */     MCD_OPC_Decode, 245, 2, 20, // Opcode: AsmOJ
/* 490 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 498
/* 494 */     MCD_OPC_Decode, 174, 1, 20, // Opcode: AsmHJ
/* 498 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 506
/* 502 */     MCD_OPC_Decode, 210, 2, 20, // Opcode: AsmNLEJ
/* 506 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 514
/* 510 */     MCD_OPC_Decode, 169, 2, 20, // Opcode: AsmLJ
/* 514 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 522
/* 518 */     MCD_OPC_Decode, 192, 2, 20, // Opcode: AsmNHEJ
/* 522 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 530
/* 526 */     MCD_OPC_Decode, 161, 2, 20, // Opcode: AsmLHJ
/* 530 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 538
/* 534 */     MCD_OPC_Decode, 182, 2, 20, // Opcode: AsmNEJ
/* 538 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 546
/* 542 */     MCD_OPC_Decode, 156, 1, 20, // Opcode: AsmEJ
/* 546 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 554
/* 550 */     MCD_OPC_Decode, 219, 2, 20, // Opcode: AsmNLHJ
/* 554 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 562
/* 558 */     MCD_OPC_Decode, 166, 1, 20, // Opcode: AsmHEJ
/* 562 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 570
/* 566 */     MCD_OPC_Decode, 227, 2, 20, // Opcode: AsmNLJ
/* 570 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 578
/* 574 */     MCD_OPC_Decode, 152, 2, 20, // Opcode: AsmLEJ
/* 578 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 586
/* 582 */     MCD_OPC_Decode, 200, 2, 20, // Opcode: AsmNHJ
/* 586 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 594
/* 590 */     MCD_OPC_Decode, 236, 2, 20, // Opcode: AsmNOJ
/* 594 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 602
/* 598 */     MCD_OPC_Decode, 167, 4, 20, // Opcode: J
/* 602 */     MCD_OPC_Decode, 145, 1, 21, // Opcode: AsmBRC
/* 606 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 614
/* 610 */     MCD_OPC_Decode, 129, 3, 22, // Opcode: BRAS
/* 614 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 622
/* 618 */     MCD_OPC_Decode, 133, 3, 23, // Opcode: BRCT
/* 622 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 630
/* 626 */     MCD_OPC_Decode, 134, 3, 24, // Opcode: BRCTG
/* 630 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 638
/* 634 */     MCD_OPC_Decode, 226, 4, 25, // Opcode: LHI
/* 638 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 646
/* 642 */     MCD_OPC_Decode, 219, 4, 26, // Opcode: LGHI
/* 646 */     MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 653
/* 650 */     MCD_OPC_Decode, 43, 27, // Opcode: AHI
/* 653 */     MCD_OPC_FilterValue, 11, 3, 0, // Skip to: 660
/* 657 */     MCD_OPC_Decode, 37, 28, // Opcode: AGHI
/* 660 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 668
/* 664 */     MCD_OPC_Decode, 188, 5, 27, // Opcode: MHI
/* 668 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 676
/* 672 */     MCD_OPC_Decode, 186, 5, 28, // Opcode: MGHI
/* 676 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 684
/* 680 */     MCD_OPC_Decode, 172, 3, 25, // Opcode: CHI
/* 684 */     MCD_OPC_FilterValue, 15, 253, 9, // Skip to: 3245
/* 688 */     MCD_OPC_Decode, 161, 3, 26, // Opcode: CGHI
/* 692 */     MCD_OPC_FilterValue, 178, 1, 68, 0, // Skip to: 765
/* 697 */     MCD_OPC_ExtractField, 8, 16,  // Inst{23-8} ...
/* 700 */     MCD_OPC_FilterValue, 128, 68, 10, 0, // Skip to: 715
/* 705 */     MCD_OPC_CheckField, 0, 4, 0, 230, 9, // Skip to: 3245
/* 711 */     MCD_OPC_Decode, 166, 4, 29, // Opcode: IPM
/* 715 */     MCD_OPC_FilterValue, 128, 158, 1, 4, 0, // Skip to: 725
/* 721 */     MCD_OPC_Decode, 138, 4, 30, // Opcode: EAR
/* 725 */     MCD_OPC_FilterValue, 128, 164, 1, 4, 0, // Skip to: 735
/* 731 */     MCD_OPC_Decode, 203, 5, 4, // Opcode: MSR
/* 735 */     MCD_OPC_FilterValue, 128, 170, 1, 4, 0, // Skip to: 745
/* 741 */     MCD_OPC_Decode, 213, 5, 31, // Opcode: MVST
/* 745 */     MCD_OPC_FilterValue, 128, 186, 1, 4, 0, // Skip to: 755
/* 751 */     MCD_OPC_Decode, 213, 3, 31, // Opcode: CLST
/* 755 */     MCD_OPC_FilterValue, 128, 188, 1, 180, 9, // Skip to: 3245
/* 761 */     MCD_OPC_Decode, 204, 6, 31, // Opcode: SRST
/* 765 */     MCD_OPC_FilterValue, 179, 1, 109, 4, // Skip to: 1903
/* 770 */     MCD_OPC_ExtractField, 16, 8,  // Inst{23-16} ...
/* 773 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 787
/* 777 */     MCD_OPC_CheckField, 8, 8, 0, 158, 9, // Skip to: 3245
/* 783 */     MCD_OPC_Decode, 142, 5, 6, // Opcode: LPEBR
/* 787 */     MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 801
/* 791 */     MCD_OPC_CheckField, 8, 8, 0, 144, 9, // Skip to: 3245
/* 797 */     MCD_OPC_Decode, 132, 5, 6, // Opcode: LNEBR
/* 801 */     MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 815
/* 805 */     MCD_OPC_CheckField, 8, 8, 0, 130, 9, // Skip to: 3245
/* 811 */     MCD_OPC_Decode, 157, 5, 6, // Opcode: LTEBR
/* 815 */     MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 829
/* 819 */     MCD_OPC_CheckField, 8, 8, 0, 116, 9, // Skip to: 3245
/* 825 */     MCD_OPC_Decode, 189, 4, 6, // Opcode: LCEBR
/* 829 */     MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 843
/* 833 */     MCD_OPC_CheckField, 8, 8, 0, 102, 9, // Skip to: 3245
/* 839 */     MCD_OPC_Decode, 196, 4, 32, // Opcode: LDEBR
/* 843 */     MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 857
/* 847 */     MCD_OPC_CheckField, 8, 8, 0, 88, 9, // Skip to: 3245
/* 853 */     MCD_OPC_Decode, 168, 5, 33, // Opcode: LXDBR
/* 857 */     MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 871
/* 861 */     MCD_OPC_CheckField, 8, 8, 0, 74, 9, // Skip to: 3245
/* 867 */     MCD_OPC_Decode, 170, 5, 34, // Opcode: LXEBR
/* 871 */     MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 885
/* 875 */     MCD_OPC_CheckField, 8, 8, 0, 60, 9, // Skip to: 3245
/* 881 */     MCD_OPC_Decode, 217, 5, 35, // Opcode: MXDBR
/* 885 */     MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 899
/* 889 */     MCD_OPC_CheckField, 8, 8, 0, 46, 9, // Skip to: 3245
/* 895 */     MCD_OPC_Decode, 143, 3, 6, // Opcode: CEBR
/* 899 */     MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 912
/* 903 */     MCD_OPC_CheckField, 8, 8, 0, 32, 9, // Skip to: 3245
/* 909 */     MCD_OPC_Decode, 29, 36, // Opcode: AEBR
/* 912 */     MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 926
/* 916 */     MCD_OPC_CheckField, 8, 8, 0, 19, 9, // Skip to: 3245
/* 922 */     MCD_OPC_Decode, 165, 6, 36, // Opcode: SEBR
/* 926 */     MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 940
/* 930 */     MCD_OPC_CheckField, 8, 8, 0, 5, 9, // Skip to: 3245
/* 936 */     MCD_OPC_Decode, 183, 5, 37, // Opcode: MDEBR
/* 940 */     MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 954
/* 944 */     MCD_OPC_CheckField, 8, 8, 0, 247, 8, // Skip to: 3245
/* 950 */     MCD_OPC_Decode, 128, 4, 36, // Opcode: DEBR
/* 954 */     MCD_OPC_FilterValue, 14, 10, 0, // Skip to: 968
/* 958 */     MCD_OPC_CheckField, 8, 4, 0, 233, 8, // Skip to: 3245
/* 964 */     MCD_OPC_Decode, 179, 5, 38, // Opcode: MAEBR
/* 968 */     MCD_OPC_FilterValue, 15, 10, 0, // Skip to: 982
/* 972 */     MCD_OPC_CheckField, 8, 4, 0, 219, 8, // Skip to: 3245
/* 978 */     MCD_OPC_Decode, 196, 5, 38, // Opcode: MSEBR
/* 982 */     MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 996
/* 986 */     MCD_OPC_CheckField, 8, 8, 0, 205, 8, // Skip to: 3245
/* 992 */     MCD_OPC_Decode, 141, 5, 5, // Opcode: LPDBR
/* 996 */     MCD_OPC_FilterValue, 17, 10, 0, // Skip to: 1010
/* 1000 */    MCD_OPC_CheckField, 8, 8, 0, 191, 8, // Skip to: 3245
/* 1006 */    MCD_OPC_Decode, 131, 5, 5, // Opcode: LNDBR
/* 1010 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 1024
/* 1014 */    MCD_OPC_CheckField, 8, 8, 0, 177, 8, // Skip to: 3245
/* 1020 */    MCD_OPC_Decode, 155, 5, 5, // Opcode: LTDBR
/* 1024 */    MCD_OPC_FilterValue, 19, 10, 0, // Skip to: 1038
/* 1028 */    MCD_OPC_CheckField, 8, 8, 0, 163, 8, // Skip to: 3245
/* 1034 */    MCD_OPC_Decode, 188, 4, 5, // Opcode: LCDBR
/* 1038 */    MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 1052
/* 1042 */    MCD_OPC_CheckField, 8, 8, 0, 149, 8, // Skip to: 3245
/* 1048 */    MCD_OPC_Decode, 194, 6, 6, // Opcode: SQEBR
/* 1052 */    MCD_OPC_FilterValue, 21, 10, 0, // Skip to: 1066
/* 1056 */    MCD_OPC_CheckField, 8, 8, 0, 135, 8, // Skip to: 3245
/* 1062 */    MCD_OPC_Decode, 192, 6, 5, // Opcode: SQDBR
/* 1066 */    MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 1080
/* 1070 */    MCD_OPC_CheckField, 8, 8, 0, 121, 8, // Skip to: 3245
/* 1076 */    MCD_OPC_Decode, 195, 6, 39, // Opcode: SQXBR
/* 1080 */    MCD_OPC_FilterValue, 23, 10, 0, // Skip to: 1094
/* 1084 */    MCD_OPC_CheckField, 8, 8, 0, 107, 8, // Skip to: 3245
/* 1090 */    MCD_OPC_Decode, 185, 5, 36, // Opcode: MEEBR
/* 1094 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 1108
/* 1098 */    MCD_OPC_CheckField, 8, 8, 0, 93, 8, // Skip to: 3245
/* 1104 */    MCD_OPC_Decode, 137, 3, 5, // Opcode: CDBR
/* 1108 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 1121
/* 1112 */    MCD_OPC_CheckField, 8, 8, 0, 79, 8, // Skip to: 3245
/* 1118 */    MCD_OPC_Decode, 24, 40, // Opcode: ADBR
/* 1121 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 1135
/* 1125 */    MCD_OPC_CheckField, 8, 8, 0, 66, 8, // Skip to: 3245
/* 1131 */    MCD_OPC_Decode, 163, 6, 40, // Opcode: SDBR
/* 1135 */    MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 1149
/* 1139 */    MCD_OPC_CheckField, 8, 8, 0, 52, 8, // Skip to: 3245
/* 1145 */    MCD_OPC_Decode, 181, 5, 40, // Opcode: MDBR
/* 1149 */    MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 1163
/* 1153 */    MCD_OPC_CheckField, 8, 8, 0, 38, 8, // Skip to: 3245
/* 1159 */    MCD_OPC_Decode, 254, 3, 40, // Opcode: DDBR
/* 1163 */    MCD_OPC_FilterValue, 30, 10, 0, // Skip to: 1177
/* 1167 */    MCD_OPC_CheckField, 8, 4, 0, 24, 8, // Skip to: 3245
/* 1173 */    MCD_OPC_Decode, 177, 5, 41, // Opcode: MADBR
/* 1177 */    MCD_OPC_FilterValue, 31, 10, 0, // Skip to: 1191
/* 1181 */    MCD_OPC_CheckField, 8, 4, 0, 10, 8, // Skip to: 3245
/* 1187 */    MCD_OPC_Decode, 194, 5, 41, // Opcode: MSDBR
/* 1191 */    MCD_OPC_FilterValue, 64, 10, 0, // Skip to: 1205
/* 1195 */    MCD_OPC_CheckField, 8, 8, 0, 252, 7, // Skip to: 3245
/* 1201 */    MCD_OPC_Decode, 146, 5, 39, // Opcode: LPXBR
/* 1205 */    MCD_OPC_FilterValue, 65, 10, 0, // Skip to: 1219
/* 1209 */    MCD_OPC_CheckField, 8, 8, 0, 238, 7, // Skip to: 3245
/* 1215 */    MCD_OPC_Decode, 136, 5, 39, // Opcode: LNXBR
/* 1219 */    MCD_OPC_FilterValue, 66, 10, 0, // Skip to: 1233
/* 1223 */    MCD_OPC_CheckField, 8, 8, 0, 224, 7, // Skip to: 3245
/* 1229 */    MCD_OPC_Decode, 164, 5, 39, // Opcode: LTXBR
/* 1233 */    MCD_OPC_FilterValue, 67, 10, 0, // Skip to: 1247
/* 1237 */    MCD_OPC_CheckField, 8, 8, 0, 210, 7, // Skip to: 3245
/* 1243 */    MCD_OPC_Decode, 193, 4, 39, // Opcode: LCXBR
/* 1247 */    MCD_OPC_FilterValue, 68, 18, 0, // Skip to: 1269
/* 1251 */    MCD_OPC_CheckField, 8, 8, 0, 4, 0, // Skip to: 1261
/* 1257 */    MCD_OPC_Decode, 203, 4, 42, // Opcode: LEDBR
/* 1261 */    MCD_OPC_CheckPredicate, 0, 188, 7, // Skip to: 3245
/* 1265 */    MCD_OPC_Decode, 204, 4, 43, // Opcode: LEDBRA
/* 1269 */    MCD_OPC_FilterValue, 69, 18, 0, // Skip to: 1291
/* 1273 */    MCD_OPC_CheckField, 8, 8, 0, 4, 0, // Skip to: 1283
/* 1279 */    MCD_OPC_Decode, 199, 4, 39, // Opcode: LDXBR
/* 1283 */    MCD_OPC_CheckPredicate, 0, 166, 7, // Skip to: 3245
/* 1287 */    MCD_OPC_Decode, 200, 4, 44, // Opcode: LDXBRA
/* 1291 */    MCD_OPC_FilterValue, 70, 18, 0, // Skip to: 1313
/* 1295 */    MCD_OPC_CheckField, 8, 8, 0, 4, 0, // Skip to: 1305
/* 1301 */    MCD_OPC_Decode, 206, 4, 39, // Opcode: LEXBR
/* 1305 */    MCD_OPC_CheckPredicate, 0, 144, 7, // Skip to: 3245
/* 1309 */    MCD_OPC_Decode, 207, 4, 44, // Opcode: LEXBRA
/* 1313 */    MCD_OPC_FilterValue, 71, 18, 0, // Skip to: 1335
/* 1317 */    MCD_OPC_CheckField, 8, 4, 0, 4, 0, // Skip to: 1327
/* 1323 */    MCD_OPC_Decode, 143, 4, 45, // Opcode: FIXBR
/* 1327 */    MCD_OPC_CheckPredicate, 0, 122, 7, // Skip to: 3245
/* 1331 */    MCD_OPC_Decode, 144, 4, 44, // Opcode: FIXBRA
/* 1335 */    MCD_OPC_FilterValue, 73, 10, 0, // Skip to: 1349
/* 1339 */    MCD_OPC_CheckField, 8, 8, 0, 108, 7, // Skip to: 3245
/* 1345 */    MCD_OPC_Decode, 227, 3, 39, // Opcode: CXBR
/* 1349 */    MCD_OPC_FilterValue, 74, 10, 0, // Skip to: 1363
/* 1353 */    MCD_OPC_CheckField, 8, 8, 0, 94, 7, // Skip to: 3245
/* 1359 */    MCD_OPC_Decode, 142, 1, 46, // Opcode: AXBR
/* 1363 */    MCD_OPC_FilterValue, 75, 10, 0, // Skip to: 1377
/* 1367 */    MCD_OPC_CheckField, 8, 8, 0, 80, 7, // Skip to: 3245
/* 1373 */    MCD_OPC_Decode, 233, 6, 46, // Opcode: SXBR
/* 1377 */    MCD_OPC_FilterValue, 76, 10, 0, // Skip to: 1391
/* 1381 */    MCD_OPC_CheckField, 8, 8, 0, 66, 7, // Skip to: 3245
/* 1387 */    MCD_OPC_Decode, 215, 5, 46, // Opcode: MXBR
/* 1391 */    MCD_OPC_FilterValue, 77, 10, 0, // Skip to: 1405
/* 1395 */    MCD_OPC_CheckField, 8, 8, 0, 52, 7, // Skip to: 3245
/* 1401 */    MCD_OPC_Decode, 137, 4, 46, // Opcode: DXBR
/* 1405 */    MCD_OPC_FilterValue, 87, 18, 0, // Skip to: 1427
/* 1409 */    MCD_OPC_CheckField, 8, 4, 0, 4, 0, // Skip to: 1419
/* 1415 */    MCD_OPC_Decode, 141, 4, 47, // Opcode: FIEBR
/* 1419 */    MCD_OPC_CheckPredicate, 0, 30, 7, // Skip to: 3245
/* 1423 */    MCD_OPC_Decode, 142, 4, 48, // Opcode: FIEBRA
/* 1427 */    MCD_OPC_FilterValue, 95, 18, 0, // Skip to: 1449
/* 1431 */    MCD_OPC_CheckField, 8, 4, 0, 4, 0, // Skip to: 1441
/* 1437 */    MCD_OPC_Decode, 139, 4, 49, // Opcode: FIDBR
/* 1441 */    MCD_OPC_CheckPredicate, 0, 8, 7, // Skip to: 3245
/* 1445 */    MCD_OPC_Decode, 140, 4, 50, // Opcode: FIDBRA
/* 1449 */    MCD_OPC_FilterValue, 101, 10, 0, // Skip to: 1463
/* 1453 */    MCD_OPC_CheckField, 8, 8, 0, 250, 6, // Skip to: 3245
/* 1459 */    MCD_OPC_Decode, 171, 5, 39, // Opcode: LXR
/* 1463 */    MCD_OPC_FilterValue, 114, 10, 0, // Skip to: 1477
/* 1467 */    MCD_OPC_CheckField, 8, 4, 0, 236, 6, // Skip to: 3245
/* 1473 */    MCD_OPC_Decode, 217, 3, 51, // Opcode: CPSDRdd
/* 1477 */    MCD_OPC_FilterValue, 116, 16, 0, // Skip to: 1497
/* 1481 */    MCD_OPC_CheckField, 8, 8, 0, 222, 6, // Skip to: 3245
/* 1487 */    MCD_OPC_CheckField, 0, 4, 0, 216, 6, // Skip to: 3245
/* 1493 */    MCD_OPC_Decode, 174, 5, 52, // Opcode: LZER
/* 1497 */    MCD_OPC_FilterValue, 117, 16, 0, // Skip to: 1517
/* 1501 */    MCD_OPC_CheckField, 8, 8, 0, 202, 6, // Skip to: 3245
/* 1507 */    MCD_OPC_CheckField, 0, 4, 0, 196, 6, // Skip to: 3245
/* 1513 */    MCD_OPC_Decode, 173, 5, 53, // Opcode: LZDR
/* 1517 */    MCD_OPC_FilterValue, 118, 16, 0, // Skip to: 1537
/* 1521 */    MCD_OPC_CheckField, 8, 8, 0, 182, 6, // Skip to: 3245
/* 1527 */    MCD_OPC_CheckField, 0, 4, 0, 176, 6, // Skip to: 3245
/* 1533 */    MCD_OPC_Decode, 175, 5, 54, // Opcode: LZXR
/* 1537 */    MCD_OPC_FilterValue, 144, 1, 8, 0, // Skip to: 1550
/* 1542 */    MCD_OPC_CheckPredicate, 0, 163, 6, // Skip to: 3245
/* 1546 */    MCD_OPC_Decode, 146, 3, 55, // Opcode: CELFBR
/* 1550 */    MCD_OPC_FilterValue, 145, 1, 8, 0, // Skip to: 1563
/* 1555 */    MCD_OPC_CheckPredicate, 0, 150, 6, // Skip to: 3245
/* 1559 */    MCD_OPC_Decode, 140, 3, 56, // Opcode: CDLFBR
/* 1563 */    MCD_OPC_FilterValue, 146, 1, 8, 0, // Skip to: 1576
/* 1568 */    MCD_OPC_CheckPredicate, 0, 137, 6, // Skip to: 3245
/* 1572 */    MCD_OPC_Decode, 230, 3, 57, // Opcode: CXLFBR
/* 1576 */    MCD_OPC_FilterValue, 148, 1, 10, 0, // Skip to: 1591
/* 1581 */    MCD_OPC_CheckField, 8, 8, 0, 122, 6, // Skip to: 3245
/* 1587 */    MCD_OPC_Decode, 144, 3, 58, // Opcode: CEFBR
/* 1591 */    MCD_OPC_FilterValue, 149, 1, 10, 0, // Skip to: 1606
/* 1596 */    MCD_OPC_CheckField, 8, 8, 0, 107, 6, // Skip to: 3245
/* 1602 */    MCD_OPC_Decode, 138, 3, 59, // Opcode: CDFBR
/* 1606 */    MCD_OPC_FilterValue, 150, 1, 10, 0, // Skip to: 1621
/* 1611 */    MCD_OPC_CheckField, 8, 8, 0, 92, 6, // Skip to: 3245
/* 1617 */    MCD_OPC_Decode, 228, 3, 60, // Opcode: CXFBR
/* 1621 */    MCD_OPC_FilterValue, 152, 1, 10, 0, // Skip to: 1636
/* 1626 */    MCD_OPC_CheckField, 8, 4, 0, 77, 6, // Skip to: 3245
/* 1632 */    MCD_OPC_Decode, 149, 3, 61, // Opcode: CFEBR
/* 1636 */    MCD_OPC_FilterValue, 153, 1, 10, 0, // Skip to: 1651
/* 1641 */    MCD_OPC_CheckField, 8, 4, 0, 62, 6, // Skip to: 3245
/* 1647 */    MCD_OPC_Decode, 148, 3, 62, // Opcode: CFDBR
/* 1651 */    MCD_OPC_FilterValue, 154, 1, 10, 0, // Skip to: 1666
/* 1656 */    MCD_OPC_CheckField, 8, 4, 0, 47, 6, // Skip to: 3245
/* 1662 */    MCD_OPC_Decode, 152, 3, 63, // Opcode: CFXBR
/* 1666 */    MCD_OPC_FilterValue, 156, 1, 8, 0, // Skip to: 1679
/* 1671 */    MCD_OPC_CheckPredicate, 0, 34, 6, // Skip to: 3245
/* 1675 */    MCD_OPC_Decode, 183, 3, 64, // Opcode: CLFEBR
/* 1679 */    MCD_OPC_FilterValue, 157, 1, 8, 0, // Skip to: 1692
/* 1684 */    MCD_OPC_CheckPredicate, 0, 21, 6, // Skip to: 3245
/* 1688 */    MCD_OPC_Decode, 182, 3, 65, // Opcode: CLFDBR
/* 1692 */    MCD_OPC_FilterValue, 158, 1, 8, 0, // Skip to: 1705
/* 1697 */    MCD_OPC_CheckPredicate, 0, 8, 6, // Skip to: 3245
/* 1701 */    MCD_OPC_Decode, 187, 3, 66, // Opcode: CLFXBR
/* 1705 */    MCD_OPC_FilterValue, 160, 1, 8, 0, // Skip to: 1718
/* 1710 */    MCD_OPC_CheckPredicate, 0, 251, 5, // Skip to: 3245
/* 1714 */    MCD_OPC_Decode, 147, 3, 67, // Opcode: CELGBR
/* 1718 */    MCD_OPC_FilterValue, 161, 1, 8, 0, // Skip to: 1731
/* 1723 */    MCD_OPC_CheckPredicate, 0, 238, 5, // Skip to: 3245
/* 1727 */    MCD_OPC_Decode, 141, 3, 68, // Opcode: CDLGBR
/* 1731 */    MCD_OPC_FilterValue, 162, 1, 8, 0, // Skip to: 1744
/* 1736 */    MCD_OPC_CheckPredicate, 0, 225, 5, // Skip to: 3245
/* 1740 */    MCD_OPC_Decode, 231, 3, 69, // Opcode: CXLGBR
/* 1744 */    MCD_OPC_FilterValue, 164, 1, 10, 0, // Skip to: 1759
/* 1749 */    MCD_OPC_CheckField, 8, 8, 0, 210, 5, // Skip to: 3245
/* 1755 */    MCD_OPC_Decode, 145, 3, 70, // Opcode: CEGBR
/* 1759 */    MCD_OPC_FilterValue, 165, 1, 10, 0, // Skip to: 1774
/* 1764 */    MCD_OPC_CheckField, 8, 8, 0, 195, 5, // Skip to: 3245
/* 1770 */    MCD_OPC_Decode, 139, 3, 71, // Opcode: CDGBR
/* 1774 */    MCD_OPC_FilterValue, 166, 1, 10, 0, // Skip to: 1789
/* 1779 */    MCD_OPC_CheckField, 8, 8, 0, 180, 5, // Skip to: 3245
/* 1785 */    MCD_OPC_Decode, 229, 3, 72, // Opcode: CXGBR
/* 1789 */    MCD_OPC_FilterValue, 168, 1, 10, 0, // Skip to: 1804
/* 1794 */    MCD_OPC_CheckField, 8, 4, 0, 165, 5, // Skip to: 3245
/* 1800 */    MCD_OPC_Decode, 155, 3, 73, // Opcode: CGEBR
/* 1804 */    MCD_OPC_FilterValue, 169, 1, 10, 0, // Skip to: 1819
/* 1809 */    MCD_OPC_CheckField, 8, 4, 0, 150, 5, // Skip to: 3245
/* 1815 */    MCD_OPC_Decode, 154, 3, 74, // Opcode: CGDBR
/* 1819 */    MCD_OPC_FilterValue, 170, 1, 10, 0, // Skip to: 1834
/* 1824 */    MCD_OPC_CheckField, 8, 4, 0, 135, 5, // Skip to: 3245
/* 1830 */    MCD_OPC_Decode, 168, 3, 75, // Opcode: CGXBR
/* 1834 */    MCD_OPC_FilterValue, 172, 1, 8, 0, // Skip to: 1847
/* 1839 */    MCD_OPC_CheckPredicate, 0, 122, 5, // Skip to: 3245
/* 1843 */    MCD_OPC_Decode, 190, 3, 76, // Opcode: CLGEBR
/* 1847 */    MCD_OPC_FilterValue, 173, 1, 8, 0, // Skip to: 1860
/* 1852 */    MCD_OPC_CheckPredicate, 0, 109, 5, // Skip to: 3245
/* 1856 */    MCD_OPC_Decode, 189, 3, 77, // Opcode: CLGDBR
/* 1860 */    MCD_OPC_FilterValue, 174, 1, 8, 0, // Skip to: 1873
/* 1865 */    MCD_OPC_CheckPredicate, 0, 96, 5, // Skip to: 3245
/* 1869 */    MCD_OPC_Decode, 201, 3, 78, // Opcode: CLGXBR
/* 1873 */    MCD_OPC_FilterValue, 193, 1, 10, 0, // Skip to: 1888
/* 1878 */    MCD_OPC_CheckField, 8, 8, 0, 81, 5, // Skip to: 3245
/* 1884 */    MCD_OPC_Decode, 197, 4, 71, // Opcode: LDGR
/* 1888 */    MCD_OPC_FilterValue, 205, 1, 72, 5, // Skip to: 3245
/* 1893 */    MCD_OPC_CheckField, 8, 8, 0, 66, 5, // Skip to: 3245
/* 1899 */    MCD_OPC_Decode, 213, 4, 79, // Opcode: LGDR
/* 1903 */    MCD_OPC_FilterValue, 185, 1, 48, 5, // Skip to: 3236
/* 1908 */    MCD_OPC_ExtractField, 16, 8,  // Inst{23-16} ...
/* 1911 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1925
/* 1915 */    MCD_OPC_CheckField, 8, 8, 0, 44, 5, // Skip to: 3245
/* 1921 */    MCD_OPC_Decode, 144, 5, 80, // Opcode: LPGR
/* 1925 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 1939
/* 1929 */    MCD_OPC_CheckField, 8, 8, 0, 30, 5, // Skip to: 3245
/* 1935 */    MCD_OPC_Decode, 134, 5, 80, // Opcode: LNGR
/* 1939 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 1953
/* 1943 */    MCD_OPC_CheckField, 8, 8, 0, 16, 5, // Skip to: 3245
/* 1949 */    MCD_OPC_Decode, 162, 5, 80, // Opcode: LTGR
/* 1953 */    MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 1967
/* 1957 */    MCD_OPC_CheckField, 8, 8, 0, 2, 5, // Skip to: 3245
/* 1963 */    MCD_OPC_Decode, 191, 4, 80, // Opcode: LCGR
/* 1967 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 1981
/* 1971 */    MCD_OPC_CheckField, 8, 8, 0, 244, 4, // Skip to: 3245
/* 1977 */    MCD_OPC_Decode, 222, 4, 80, // Opcode: LGR
/* 1981 */    MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 1995
/* 1985 */    MCD_OPC_CheckField, 8, 8, 0, 230, 4, // Skip to: 3245
/* 1991 */    MCD_OPC_Decode, 212, 4, 80, // Opcode: LGBR
/* 1995 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 2009
/* 1999 */    MCD_OPC_CheckField, 8, 8, 0, 216, 4, // Skip to: 3245
/* 2005 */    MCD_OPC_Decode, 220, 4, 80, // Opcode: LGHR
/* 2009 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 2022
/* 2013 */    MCD_OPC_CheckField, 8, 8, 0, 202, 4, // Skip to: 3245
/* 2019 */    MCD_OPC_Decode, 39, 81, // Opcode: AGR
/* 2022 */    MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 2036
/* 2026 */    MCD_OPC_CheckField, 8, 8, 0, 189, 4, // Skip to: 3245
/* 2032 */    MCD_OPC_Decode, 169, 6, 81, // Opcode: SGR
/* 2036 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 2049
/* 2040 */    MCD_OPC_CheckField, 8, 8, 0, 175, 4, // Skip to: 3245
/* 2046 */    MCD_OPC_Decode, 60, 81, // Opcode: ALGR
/* 2049 */    MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 2063
/* 2053 */    MCD_OPC_CheckField, 8, 8, 0, 162, 4, // Skip to: 3245
/* 2059 */    MCD_OPC_Decode, 183, 6, 81, // Opcode: SLGR
/* 2063 */    MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 2077
/* 2067 */    MCD_OPC_CheckField, 8, 8, 0, 148, 4, // Skip to: 3245
/* 2073 */    MCD_OPC_Decode, 202, 5, 81, // Opcode: MSGR
/* 2077 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 2091
/* 2081 */    MCD_OPC_CheckField, 8, 8, 0, 134, 4, // Skip to: 3245
/* 2087 */    MCD_OPC_Decode, 136, 4, 82, // Opcode: DSGR
/* 2091 */    MCD_OPC_FilterValue, 15, 10, 0, // Skip to: 2105
/* 2095 */    MCD_OPC_CheckField, 8, 8, 0, 120, 4, // Skip to: 3245
/* 2101 */    MCD_OPC_Decode, 152, 5, 80, // Opcode: LRVGR
/* 2105 */    MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 2119
/* 2109 */    MCD_OPC_CheckField, 8, 8, 0, 106, 4, // Skip to: 3245
/* 2115 */    MCD_OPC_Decode, 143, 5, 83, // Opcode: LPGFR
/* 2119 */    MCD_OPC_FilterValue, 17, 10, 0, // Skip to: 2133
/* 2123 */    MCD_OPC_CheckField, 8, 8, 0, 92, 4, // Skip to: 3245
/* 2129 */    MCD_OPC_Decode, 133, 5, 83, // Opcode: LNGFR
/* 2133 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 2147
/* 2137 */    MCD_OPC_CheckField, 8, 8, 0, 78, 4, // Skip to: 3245
/* 2143 */    MCD_OPC_Decode, 161, 5, 80, // Opcode: LTGFR
/* 2147 */    MCD_OPC_FilterValue, 19, 10, 0, // Skip to: 2161
/* 2151 */    MCD_OPC_CheckField, 8, 8, 0, 64, 4, // Skip to: 3245
/* 2157 */    MCD_OPC_Decode, 190, 4, 83, // Opcode: LCGFR
/* 2161 */    MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 2175
/* 2165 */    MCD_OPC_CheckField, 8, 8, 0, 50, 4, // Skip to: 3245
/* 2171 */    MCD_OPC_Decode, 216, 4, 83, // Opcode: LGFR
/* 2175 */    MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 2189
/* 2179 */    MCD_OPC_CheckField, 8, 8, 0, 36, 4, // Skip to: 3245
/* 2185 */    MCD_OPC_Decode, 240, 4, 83, // Opcode: LLGFR
/* 2189 */    MCD_OPC_FilterValue, 24, 9, 0, // Skip to: 2202
/* 2193 */    MCD_OPC_CheckField, 8, 8, 0, 22, 4, // Skip to: 3245
/* 2199 */    MCD_OPC_Decode, 36, 84, // Opcode: AGFR
/* 2202 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 2216
/* 2206 */    MCD_OPC_CheckField, 8, 8, 0, 9, 4, // Skip to: 3245
/* 2212 */    MCD_OPC_Decode, 168, 6, 84, // Opcode: SGFR
/* 2216 */    MCD_OPC_FilterValue, 26, 9, 0, // Skip to: 2229
/* 2220 */    MCD_OPC_CheckField, 8, 8, 0, 251, 3, // Skip to: 3245
/* 2226 */    MCD_OPC_Decode, 58, 84, // Opcode: ALGFR
/* 2229 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 2243
/* 2233 */    MCD_OPC_CheckField, 8, 8, 0, 238, 3, // Skip to: 3245
/* 2239 */    MCD_OPC_Decode, 182, 6, 84, // Opcode: SLGFR
/* 2243 */    MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 2257
/* 2247 */    MCD_OPC_CheckField, 8, 8, 0, 224, 3, // Skip to: 3245
/* 2253 */    MCD_OPC_Decode, 201, 5, 84, // Opcode: MSGFR
/* 2257 */    MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 2271
/* 2261 */    MCD_OPC_CheckField, 8, 8, 0, 210, 3, // Skip to: 3245
/* 2267 */    MCD_OPC_Decode, 135, 4, 85, // Opcode: DSGFR
/* 2271 */    MCD_OPC_FilterValue, 31, 10, 0, // Skip to: 2285
/* 2275 */    MCD_OPC_CheckField, 8, 8, 0, 196, 3, // Skip to: 3245
/* 2281 */    MCD_OPC_Decode, 153, 5, 3, // Opcode: LRVR
/* 2285 */    MCD_OPC_FilterValue, 32, 10, 0, // Skip to: 2299
/* 2289 */    MCD_OPC_CheckField, 8, 8, 0, 182, 3, // Skip to: 3245
/* 2295 */    MCD_OPC_Decode, 165, 3, 80, // Opcode: CGR
/* 2299 */    MCD_OPC_FilterValue, 33, 10, 0, // Skip to: 2313
/* 2303 */    MCD_OPC_CheckField, 8, 8, 0, 168, 3, // Skip to: 3245
/* 2309 */    MCD_OPC_Decode, 198, 3, 80, // Opcode: CLGR
/* 2313 */    MCD_OPC_FilterValue, 38, 10, 0, // Skip to: 2327
/* 2317 */    MCD_OPC_CheckField, 8, 8, 0, 154, 3, // Skip to: 3245
/* 2323 */    MCD_OPC_Decode, 187, 4, 3, // Opcode: LBR
/* 2327 */    MCD_OPC_FilterValue, 39, 10, 0, // Skip to: 2341
/* 2331 */    MCD_OPC_CheckField, 8, 8, 0, 140, 3, // Skip to: 3245
/* 2337 */    MCD_OPC_Decode, 229, 4, 3, // Opcode: LHR
/* 2341 */    MCD_OPC_FilterValue, 48, 10, 0, // Skip to: 2355
/* 2345 */    MCD_OPC_CheckField, 8, 8, 0, 126, 3, // Skip to: 3245
/* 2351 */    MCD_OPC_Decode, 158, 3, 83, // Opcode: CGFR
/* 2355 */    MCD_OPC_FilterValue, 49, 10, 0, // Skip to: 2369
/* 2359 */    MCD_OPC_CheckField, 8, 8, 0, 112, 3, // Skip to: 3245
/* 2365 */    MCD_OPC_Decode, 193, 3, 83, // Opcode: CLGFR
/* 2369 */    MCD_OPC_FilterValue, 128, 1, 10, 0, // Skip to: 2384
/* 2374 */    MCD_OPC_CheckField, 8, 8, 0, 97, 3, // Skip to: 3245
/* 2380 */    MCD_OPC_Decode, 223, 5, 81, // Opcode: NGR
/* 2384 */    MCD_OPC_FilterValue, 129, 1, 10, 0, // Skip to: 2399
/* 2389 */    MCD_OPC_CheckField, 8, 8, 0, 82, 3, // Skip to: 3245
/* 2395 */    MCD_OPC_Decode, 250, 5, 81, // Opcode: OGR
/* 2399 */    MCD_OPC_FilterValue, 130, 1, 10, 0, // Skip to: 2414
/* 2404 */    MCD_OPC_CheckField, 8, 8, 0, 67, 3, // Skip to: 3245
/* 2410 */    MCD_OPC_Decode, 133, 7, 81, // Opcode: XGR
/* 2414 */    MCD_OPC_FilterValue, 131, 1, 10, 0, // Skip to: 2429
/* 2419 */    MCD_OPC_CheckField, 8, 8, 0, 52, 3, // Skip to: 3245
/* 2425 */    MCD_OPC_Decode, 145, 4, 86, // Opcode: FLOGR
/* 2429 */    MCD_OPC_FilterValue, 132, 1, 10, 0, // Skip to: 2444
/* 2434 */    MCD_OPC_CheckField, 8, 8, 0, 37, 3, // Skip to: 3245
/* 2440 */    MCD_OPC_Decode, 238, 4, 80, // Opcode: LLGCR
/* 2444 */    MCD_OPC_FilterValue, 133, 1, 10, 0, // Skip to: 2459
/* 2449 */    MCD_OPC_CheckField, 8, 8, 0, 22, 3, // Skip to: 3245
/* 2455 */    MCD_OPC_Decode, 243, 4, 80, // Opcode: LLGHR
/* 2459 */    MCD_OPC_FilterValue, 134, 1, 10, 0, // Skip to: 2474
/* 2464 */    MCD_OPC_CheckField, 8, 8, 0, 7, 3, // Skip to: 3245
/* 2470 */    MCD_OPC_Decode, 191, 5, 82, // Opcode: MLGR
/* 2474 */    MCD_OPC_FilterValue, 135, 1, 10, 0, // Skip to: 2489
/* 2479 */    MCD_OPC_CheckField, 8, 8, 0, 248, 2, // Skip to: 3245
/* 2485 */    MCD_OPC_Decode, 131, 4, 82, // Opcode: DLGR
/* 2489 */    MCD_OPC_FilterValue, 136, 1, 9, 0, // Skip to: 2503
/* 2494 */    MCD_OPC_CheckField, 8, 8, 0, 233, 2, // Skip to: 3245
/* 2500 */    MCD_OPC_Decode, 52, 81, // Opcode: ALCGR
/* 2503 */    MCD_OPC_FilterValue, 137, 1, 10, 0, // Skip to: 2518
/* 2508 */    MCD_OPC_CheckField, 8, 8, 0, 219, 2, // Skip to: 3245
/* 2514 */    MCD_OPC_Decode, 179, 6, 81, // Opcode: SLGBR
/* 2518 */    MCD_OPC_FilterValue, 148, 1, 10, 0, // Skip to: 2533
/* 2523 */    MCD_OPC_CheckField, 8, 8, 0, 204, 2, // Skip to: 3245
/* 2529 */    MCD_OPC_Decode, 235, 4, 3, // Opcode: LLCR
/* 2533 */    MCD_OPC_FilterValue, 149, 1, 10, 0, // Skip to: 2548
/* 2538 */    MCD_OPC_CheckField, 8, 8, 0, 189, 2, // Skip to: 3245
/* 2544 */    MCD_OPC_Decode, 248, 4, 3, // Opcode: LLHR
/* 2548 */    MCD_OPC_FilterValue, 151, 1, 10, 0, // Skip to: 2563
/* 2553 */    MCD_OPC_CheckField, 8, 8, 0, 174, 2, // Skip to: 3245
/* 2559 */    MCD_OPC_Decode, 132, 4, 85, // Opcode: DLR
/* 2563 */    MCD_OPC_FilterValue, 152, 1, 9, 0, // Skip to: 2577
/* 2568 */    MCD_OPC_CheckField, 8, 8, 0, 159, 2, // Skip to: 3245
/* 2574 */    MCD_OPC_Decode, 53, 4, // Opcode: ALCR
/* 2577 */    MCD_OPC_FilterValue, 153, 1, 10, 0, // Skip to: 2592
/* 2582 */    MCD_OPC_CheckField, 8, 8, 0, 145, 2, // Skip to: 3245
/* 2588 */    MCD_OPC_Decode, 176, 6, 4, // Opcode: SLBR
/* 2592 */    MCD_OPC_FilterValue, 226, 1, 186, 0, // Skip to: 2783
/* 2597 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2600 */    MCD_OPC_FilterValue, 0, 129, 2, // Skip to: 3245
/* 2604 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2607 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 2619
/* 2611 */    MCD_OPC_CheckPredicate, 1, 160, 0, // Skip to: 2775
/* 2615 */    MCD_OPC_Decode, 249, 2, 81, // Opcode: AsmOLOCGR
/* 2619 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 2631
/* 2623 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 2775
/* 2627 */    MCD_OPC_Decode, 178, 1, 81, // Opcode: AsmHLOCGR
/* 2631 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 2643
/* 2635 */    MCD_OPC_CheckPredicate, 1, 136, 0, // Skip to: 2775
/* 2639 */    MCD_OPC_Decode, 214, 2, 81, // Opcode: AsmNLELOCGR
/* 2643 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 2655
/* 2647 */    MCD_OPC_CheckPredicate, 1, 124, 0, // Skip to: 2775
/* 2651 */    MCD_OPC_Decode, 173, 2, 81, // Opcode: AsmLLOCGR
/* 2655 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 2667
/* 2659 */    MCD_OPC_CheckPredicate, 1, 112, 0, // Skip to: 2775
/* 2663 */    MCD_OPC_Decode, 196, 2, 81, // Opcode: AsmNHELOCGR
/* 2667 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 2679
/* 2671 */    MCD_OPC_CheckPredicate, 1, 100, 0, // Skip to: 2775
/* 2675 */    MCD_OPC_Decode, 165, 2, 81, // Opcode: AsmLHLOCGR
/* 2679 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 2691
/* 2683 */    MCD_OPC_CheckPredicate, 1, 88, 0, // Skip to: 2775
/* 2687 */    MCD_OPC_Decode, 186, 2, 81, // Opcode: AsmNELOCGR
/* 2691 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 2703
/* 2695 */    MCD_OPC_CheckPredicate, 1, 76, 0, // Skip to: 2775
/* 2699 */    MCD_OPC_Decode, 160, 1, 81, // Opcode: AsmELOCGR
/* 2703 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 2715
/* 2707 */    MCD_OPC_CheckPredicate, 1, 64, 0, // Skip to: 2775
/* 2711 */    MCD_OPC_Decode, 223, 2, 81, // Opcode: AsmNLHLOCGR
/* 2715 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 2727
/* 2719 */    MCD_OPC_CheckPredicate, 1, 52, 0, // Skip to: 2775
/* 2723 */    MCD_OPC_Decode, 170, 1, 81, // Opcode: AsmHELOCGR
/* 2727 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 2739
/* 2731 */    MCD_OPC_CheckPredicate, 1, 40, 0, // Skip to: 2775
/* 2735 */    MCD_OPC_Decode, 231, 2, 81, // Opcode: AsmNLLOCGR
/* 2739 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 2751
/* 2743 */    MCD_OPC_CheckPredicate, 1, 28, 0, // Skip to: 2775
/* 2747 */    MCD_OPC_Decode, 156, 2, 81, // Opcode: AsmLELOCGR
/* 2751 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 2763
/* 2755 */    MCD_OPC_CheckPredicate, 1, 16, 0, // Skip to: 2775
/* 2759 */    MCD_OPC_Decode, 204, 2, 81, // Opcode: AsmNHLOCGR
/* 2763 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 2775
/* 2767 */    MCD_OPC_CheckPredicate, 1, 4, 0, // Skip to: 2775
/* 2771 */    MCD_OPC_Decode, 240, 2, 81, // Opcode: AsmNOLOCGR
/* 2775 */    MCD_OPC_CheckPredicate, 1, 210, 1, // Skip to: 3245
/* 2779 */    MCD_OPC_Decode, 177, 2, 87, // Opcode: AsmLOCGR
/* 2783 */    MCD_OPC_FilterValue, 228, 1, 14, 0, // Skip to: 2802
/* 2788 */    MCD_OPC_CheckPredicate, 2, 197, 1, // Skip to: 3245
/* 2792 */    MCD_OPC_CheckField, 8, 4, 0, 191, 1, // Skip to: 3245
/* 2798 */    MCD_OPC_Decode, 224, 5, 88, // Opcode: NGRK
/* 2802 */    MCD_OPC_FilterValue, 230, 1, 14, 0, // Skip to: 2821
/* 2807 */    MCD_OPC_CheckPredicate, 2, 178, 1, // Skip to: 3245
/* 2811 */    MCD_OPC_CheckField, 8, 4, 0, 172, 1, // Skip to: 3245
/* 2817 */    MCD_OPC_Decode, 251, 5, 88, // Opcode: OGRK
/* 2821 */    MCD_OPC_FilterValue, 231, 1, 14, 0, // Skip to: 2840
/* 2826 */    MCD_OPC_CheckPredicate, 2, 159, 1, // Skip to: 3245
/* 2830 */    MCD_OPC_CheckField, 8, 4, 0, 153, 1, // Skip to: 3245
/* 2836 */    MCD_OPC_Decode, 134, 7, 88, // Opcode: XGRK
/* 2840 */    MCD_OPC_FilterValue, 232, 1, 13, 0, // Skip to: 2858
/* 2845 */    MCD_OPC_CheckPredicate, 2, 140, 1, // Skip to: 3245
/* 2849 */    MCD_OPC_CheckField, 8, 4, 0, 134, 1, // Skip to: 3245
/* 2855 */    MCD_OPC_Decode, 40, 88, // Opcode: AGRK
/* 2858 */    MCD_OPC_FilterValue, 233, 1, 14, 0, // Skip to: 2877
/* 2863 */    MCD_OPC_CheckPredicate, 2, 122, 1, // Skip to: 3245
/* 2867 */    MCD_OPC_CheckField, 8, 4, 0, 116, 1, // Skip to: 3245
/* 2873 */    MCD_OPC_Decode, 170, 6, 88, // Opcode: SGRK
/* 2877 */    MCD_OPC_FilterValue, 234, 1, 13, 0, // Skip to: 2895
/* 2882 */    MCD_OPC_CheckPredicate, 2, 103, 1, // Skip to: 3245
/* 2886 */    MCD_OPC_CheckField, 8, 4, 0, 97, 1, // Skip to: 3245
/* 2892 */    MCD_OPC_Decode, 61, 88, // Opcode: ALGRK
/* 2895 */    MCD_OPC_FilterValue, 235, 1, 14, 0, // Skip to: 2914
/* 2900 */    MCD_OPC_CheckPredicate, 2, 85, 1, // Skip to: 3245
/* 2904 */    MCD_OPC_CheckField, 8, 4, 0, 79, 1, // Skip to: 3245
/* 2910 */    MCD_OPC_Decode, 184, 6, 88, // Opcode: SLGRK
/* 2914 */    MCD_OPC_FilterValue, 242, 1, 186, 0, // Skip to: 3105
/* 2919 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2922 */    MCD_OPC_FilterValue, 0, 63, 1, // Skip to: 3245
/* 2926 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2929 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 2941
/* 2933 */    MCD_OPC_CheckPredicate, 1, 160, 0, // Skip to: 3097
/* 2937 */    MCD_OPC_Decode, 250, 2, 4, // Opcode: AsmOLOCR
/* 2941 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 2953
/* 2945 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 3097
/* 2949 */    MCD_OPC_Decode, 179, 1, 4, // Opcode: AsmHLOCR
/* 2953 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 2965
/* 2957 */    MCD_OPC_CheckPredicate, 1, 136, 0, // Skip to: 3097
/* 2961 */    MCD_OPC_Decode, 215, 2, 4, // Opcode: AsmNLELOCR
/* 2965 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 2977
/* 2969 */    MCD_OPC_CheckPredicate, 1, 124, 0, // Skip to: 3097
/* 2973 */    MCD_OPC_Decode, 174, 2, 4, // Opcode: AsmLLOCR
/* 2977 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 2989
/* 2981 */    MCD_OPC_CheckPredicate, 1, 112, 0, // Skip to: 3097
/* 2985 */    MCD_OPC_Decode, 197, 2, 4, // Opcode: AsmNHELOCR
/* 2989 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 3001
/* 2993 */    MCD_OPC_CheckPredicate, 1, 100, 0, // Skip to: 3097
/* 2997 */    MCD_OPC_Decode, 166, 2, 4, // Opcode: AsmLHLOCR
/* 3001 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 3013
/* 3005 */    MCD_OPC_CheckPredicate, 1, 88, 0, // Skip to: 3097
/* 3009 */    MCD_OPC_Decode, 187, 2, 4, // Opcode: AsmNELOCR
/* 3013 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 3025
/* 3017 */    MCD_OPC_CheckPredicate, 1, 76, 0, // Skip to: 3097
/* 3021 */    MCD_OPC_Decode, 161, 1, 4, // Opcode: AsmELOCR
/* 3025 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 3037
/* 3029 */    MCD_OPC_CheckPredicate, 1, 64, 0, // Skip to: 3097
/* 3033 */    MCD_OPC_Decode, 224, 2, 4, // Opcode: AsmNLHLOCR
/* 3037 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 3049
/* 3041 */    MCD_OPC_CheckPredicate, 1, 52, 0, // Skip to: 3097
/* 3045 */    MCD_OPC_Decode, 171, 1, 4, // Opcode: AsmHELOCR
/* 3049 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 3061
/* 3053 */    MCD_OPC_CheckPredicate, 1, 40, 0, // Skip to: 3097
/* 3057 */    MCD_OPC_Decode, 232, 2, 4, // Opcode: AsmNLLOCR
/* 3061 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 3073
/* 3065 */    MCD_OPC_CheckPredicate, 1, 28, 0, // Skip to: 3097
/* 3069 */    MCD_OPC_Decode, 157, 2, 4, // Opcode: AsmLELOCR
/* 3073 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 3085
/* 3077 */    MCD_OPC_CheckPredicate, 1, 16, 0, // Skip to: 3097
/* 3081 */    MCD_OPC_Decode, 205, 2, 4, // Opcode: AsmNHLOCR
/* 3085 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 3097
/* 3089 */    MCD_OPC_CheckPredicate, 1, 4, 0, // Skip to: 3097
/* 3093 */    MCD_OPC_Decode, 241, 2, 4, // Opcode: AsmNOLOCR
/* 3097 */    MCD_OPC_CheckPredicate, 1, 144, 0, // Skip to: 3245
/* 3101 */    MCD_OPC_Decode, 178, 2, 89, // Opcode: AsmLOCR
/* 3105 */    MCD_OPC_FilterValue, 244, 1, 14, 0, // Skip to: 3124
/* 3110 */    MCD_OPC_CheckPredicate, 2, 131, 0, // Skip to: 3245
/* 3114 */    MCD_OPC_CheckField, 8, 4, 0, 125, 0, // Skip to: 3245
/* 3120 */    MCD_OPC_Decode, 243, 5, 90, // Opcode: NRK
/* 3124 */    MCD_OPC_FilterValue, 246, 1, 14, 0, // Skip to: 3143
/* 3129 */    MCD_OPC_CheckPredicate, 2, 112, 0, // Skip to: 3245
/* 3133 */    MCD_OPC_CheckField, 8, 4, 0, 106, 0, // Skip to: 3245
/* 3139 */    MCD_OPC_Decode, 142, 6, 90, // Opcode: ORK
/* 3143 */    MCD_OPC_FilterValue, 247, 1, 14, 0, // Skip to: 3162
/* 3148 */    MCD_OPC_CheckPredicate, 2, 93, 0, // Skip to: 3245
/* 3152 */    MCD_OPC_CheckField, 8, 4, 0, 87, 0, // Skip to: 3245
/* 3158 */    MCD_OPC_Decode, 143, 7, 90, // Opcode: XRK
/* 3162 */    MCD_OPC_FilterValue, 248, 1, 13, 0, // Skip to: 3180
/* 3167 */    MCD_OPC_CheckPredicate, 2, 74, 0, // Skip to: 3245
/* 3171 */    MCD_OPC_CheckField, 8, 4, 0, 68, 0, // Skip to: 3245
/* 3177 */    MCD_OPC_Decode, 67, 90, // Opcode: ARK
/* 3180 */    MCD_OPC_FilterValue, 249, 1, 14, 0, // Skip to: 3199
/* 3185 */    MCD_OPC_CheckPredicate, 2, 56, 0, // Skip to: 3245
/* 3189 */    MCD_OPC_CheckField, 8, 4, 0, 50, 0, // Skip to: 3245
/* 3195 */    MCD_OPC_Decode, 200, 6, 90, // Opcode: SRK
/* 3199 */    MCD_OPC_FilterValue, 250, 1, 13, 0, // Skip to: 3217
/* 3204 */    MCD_OPC_CheckPredicate, 2, 37, 0, // Skip to: 3245
/* 3208 */    MCD_OPC_CheckField, 8, 4, 0, 31, 0, // Skip to: 3245
/* 3214 */    MCD_OPC_Decode, 64, 90, // Opcode: ALRK
/* 3217 */    MCD_OPC_FilterValue, 251, 1, 23, 0, // Skip to: 3245
/* 3222 */    MCD_OPC_CheckPredicate, 2, 19, 0, // Skip to: 3245
/* 3226 */    MCD_OPC_CheckField, 8, 4, 0, 13, 0, // Skip to: 3245
/* 3232 */    MCD_OPC_Decode, 189, 6, 90, // Opcode: SLRK
/* 3236 */    MCD_OPC_FilterValue, 186, 1, 4, 0, // Skip to: 3245
/* 3241 */    MCD_OPC_Decode, 224, 3, 91, // Opcode: CS
/* 3245 */    MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTable48[] = {
/* 0 */       MCD_OPC_ExtractField, 40, 8,  // Inst{47-40} ...
/* 3 */       MCD_OPC_FilterValue, 192, 1, 238, 0, // Skip to: 246
/* 8 */       MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 11 */      MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 19
/* 15 */      MCD_OPC_Decode, 180, 4, 92, // Opcode: LARL
/* 19 */      MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 27
/* 23 */      MCD_OPC_Decode, 215, 4, 93, // Opcode: LGFI
/* 27 */      MCD_OPC_FilterValue, 4, 127, 0, // Skip to: 158
/* 31 */      MCD_OPC_ExtractField, 36, 4,  // Inst{39-36} ...
/* 34 */      MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 42
/* 38 */      MCD_OPC_Decode, 246, 2, 94, // Opcode: AsmOJG
/* 42 */      MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 50
/* 46 */      MCD_OPC_Decode, 175, 1, 94, // Opcode: AsmHJG
/* 50 */      MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 58
/* 54 */      MCD_OPC_Decode, 211, 2, 94, // Opcode: AsmNLEJG
/* 58 */      MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 66
/* 62 */      MCD_OPC_Decode, 170, 2, 94, // Opcode: AsmLJG
/* 66 */      MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 74
/* 70 */      MCD_OPC_Decode, 193, 2, 94, // Opcode: AsmNHEJG
/* 74 */      MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 82
/* 78 */      MCD_OPC_Decode, 162, 2, 94, // Opcode: AsmLHJG
/* 82 */      MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 90
/* 86 */      MCD_OPC_Decode, 183, 2, 94, // Opcode: AsmNEJG
/* 90 */      MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 98
/* 94 */      MCD_OPC_Decode, 157, 1, 94, // Opcode: AsmEJG
/* 98 */      MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 106
/* 102 */     MCD_OPC_Decode, 220, 2, 94, // Opcode: AsmNLHJG
/* 106 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 114
/* 110 */     MCD_OPC_Decode, 167, 1, 94, // Opcode: AsmHEJG
/* 114 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 122
/* 118 */     MCD_OPC_Decode, 228, 2, 94, // Opcode: AsmNLJG
/* 122 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 130
/* 126 */     MCD_OPC_Decode, 153, 2, 94, // Opcode: AsmLEJG
/* 130 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 138
/* 134 */     MCD_OPC_Decode, 201, 2, 94, // Opcode: AsmNHJG
/* 138 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 146
/* 142 */     MCD_OPC_Decode, 237, 2, 94, // Opcode: AsmNOJG
/* 146 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 154
/* 150 */     MCD_OPC_Decode, 168, 4, 94, // Opcode: JG
/* 154 */     MCD_OPC_Decode, 146, 1, 95, // Opcode: AsmBRCL
/* 158 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 166
/* 162 */     MCD_OPC_Decode, 130, 3, 92, // Opcode: BRASL
/* 166 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 174
/* 170 */     MCD_OPC_Decode, 137, 7, 96, // Opcode: XIHF
/* 174 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 182
/* 178 */     MCD_OPC_Decode, 139, 7, 97, // Opcode: XILF
/* 182 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 190
/* 186 */     MCD_OPC_Decode, 152, 4, 98, // Opcode: IIHF
/* 190 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 198
/* 194 */     MCD_OPC_Decode, 159, 4, 99, // Opcode: IILF
/* 198 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 206
/* 202 */     MCD_OPC_Decode, 227, 5, 96, // Opcode: NIHF
/* 206 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 214
/* 210 */     MCD_OPC_Decode, 234, 5, 97, // Opcode: NILF
/* 214 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 222
/* 218 */     MCD_OPC_Decode, 254, 5, 96, // Opcode: OIHF
/* 222 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 230
/* 226 */     MCD_OPC_Decode, 133, 6, 97, // Opcode: OILF
/* 230 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 238
/* 234 */     MCD_OPC_Decode, 251, 4, 100, // Opcode: LLIHF
/* 238 */     MCD_OPC_FilterValue, 15, 214, 12, // Skip to: 3528
/* 242 */     MCD_OPC_Decode, 254, 4, 100, // Opcode: LLILF
/* 246 */     MCD_OPC_FilterValue, 194, 1, 95, 0, // Skip to: 346
/* 251 */     MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 254 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 262
/* 258 */     MCD_OPC_Decode, 200, 5, 101, // Opcode: MSGFI
/* 262 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 270
/* 266 */     MCD_OPC_Decode, 197, 5, 102, // Opcode: MSFI
/* 270 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 278
/* 274 */     MCD_OPC_Decode, 181, 6, 103, // Opcode: SLGFI
/* 278 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 286
/* 282 */     MCD_OPC_Decode, 177, 6, 97, // Opcode: SLFI
/* 286 */     MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 293
/* 290 */     MCD_OPC_Decode, 35, 101, // Opcode: AGFI
/* 293 */     MCD_OPC_FilterValue, 9, 3, 0, // Skip to: 300
/* 297 */     MCD_OPC_Decode, 31, 102, // Opcode: AFI
/* 300 */     MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 307
/* 304 */     MCD_OPC_Decode, 57, 103, // Opcode: ALGFI
/* 307 */     MCD_OPC_FilterValue, 11, 3, 0, // Skip to: 314
/* 311 */     MCD_OPC_Decode, 54, 97, // Opcode: ALFI
/* 314 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 322
/* 318 */     MCD_OPC_Decode, 157, 3, 93, // Opcode: CGFI
/* 322 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 330
/* 326 */     MCD_OPC_Decode, 150, 3, 104, // Opcode: CFI
/* 330 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 338
/* 334 */     MCD_OPC_Decode, 192, 3, 100, // Opcode: CLGFI
/* 338 */     MCD_OPC_FilterValue, 15, 114, 12, // Skip to: 3528
/* 342 */     MCD_OPC_Decode, 185, 3, 99, // Opcode: CLFI
/* 346 */     MCD_OPC_FilterValue, 196, 1, 91, 0, // Skip to: 442
/* 351 */     MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 354 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 362
/* 358 */     MCD_OPC_Decode, 249, 4, 105, // Opcode: LLHRL
/* 362 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 370
/* 366 */     MCD_OPC_Decode, 221, 4, 92, // Opcode: LGHRL
/* 370 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 378
/* 374 */     MCD_OPC_Decode, 230, 4, 105, // Opcode: LHRL
/* 378 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 386
/* 382 */     MCD_OPC_Decode, 244, 4, 92, // Opcode: LLGHRL
/* 386 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 394
/* 390 */     MCD_OPC_Decode, 222, 6, 105, // Opcode: STHRL
/* 394 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 402
/* 398 */     MCD_OPC_Decode, 223, 4, 92, // Opcode: LGRL
/* 402 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 410
/* 406 */     MCD_OPC_Decode, 218, 6, 92, // Opcode: STGRL
/* 410 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 418
/* 414 */     MCD_OPC_Decode, 217, 4, 92, // Opcode: LGFRL
/* 418 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 426
/* 422 */     MCD_OPC_Decode, 148, 5, 105, // Opcode: LRL
/* 426 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 434
/* 430 */     MCD_OPC_Decode, 241, 4, 92, // Opcode: LLGFRL
/* 434 */     MCD_OPC_FilterValue, 15, 18, 12, // Skip to: 3528
/* 438 */     MCD_OPC_Decode, 228, 6, 105, // Opcode: STRL
/* 442 */     MCD_OPC_FilterValue, 198, 1, 91, 0, // Skip to: 538
/* 447 */     MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 450 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 458
/* 454 */     MCD_OPC_Decode, 145, 6, 95, // Opcode: PFDRL
/* 458 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 466
/* 462 */     MCD_OPC_Decode, 162, 3, 92, // Opcode: CGHRL
/* 466 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 474
/* 470 */     MCD_OPC_Decode, 173, 3, 105, // Opcode: CHRL
/* 474 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 482
/* 478 */     MCD_OPC_Decode, 195, 3, 92, // Opcode: CLGHRL
/* 482 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 490
/* 486 */     MCD_OPC_Decode, 204, 3, 105, // Opcode: CLHRL
/* 490 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 498
/* 494 */     MCD_OPC_Decode, 167, 3, 92, // Opcode: CGRL
/* 498 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 506
/* 502 */     MCD_OPC_Decode, 200, 3, 92, // Opcode: CLGRL
/* 506 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 514
/* 510 */     MCD_OPC_Decode, 159, 3, 92, // Opcode: CGFRL
/* 514 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 522
/* 518 */     MCD_OPC_Decode, 223, 3, 105, // Opcode: CRL
/* 522 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 530
/* 526 */     MCD_OPC_Decode, 194, 3, 92, // Opcode: CLGFRL
/* 530 */     MCD_OPC_FilterValue, 15, 178, 11, // Skip to: 3528
/* 534 */     MCD_OPC_Decode, 212, 3, 105, // Opcode: CLRL
/* 538 */     MCD_OPC_FilterValue, 204, 1, 38, 0, // Skip to: 581
/* 543 */     MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 546 */     MCD_OPC_FilterValue, 8, 7, 0, // Skip to: 557
/* 550 */     MCD_OPC_CheckPredicate, 3, 158, 11, // Skip to: 3528
/* 554 */     MCD_OPC_Decode, 48, 106, // Opcode: AIH
/* 557 */     MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 569
/* 561 */     MCD_OPC_CheckPredicate, 3, 147, 11, // Skip to: 3528
/* 565 */     MCD_OPC_Decode, 176, 3, 107, // Opcode: CIH
/* 569 */     MCD_OPC_FilterValue, 15, 139, 11, // Skip to: 3528
/* 573 */     MCD_OPC_CheckPredicate, 3, 135, 11, // Skip to: 3528
/* 577 */     MCD_OPC_Decode, 206, 3, 99, // Opcode: CLIH
/* 581 */     MCD_OPC_FilterValue, 210, 1, 4, 0, // Skip to: 590
/* 586 */     MCD_OPC_Decode, 205, 5, 108, // Opcode: MVC
/* 590 */     MCD_OPC_FilterValue, 212, 1, 4, 0, // Skip to: 599
/* 595 */     MCD_OPC_Decode, 219, 5, 108, // Opcode: NC
/* 599 */     MCD_OPC_FilterValue, 213, 1, 4, 0, // Skip to: 608
/* 604 */     MCD_OPC_Decode, 179, 3, 108, // Opcode: CLC
/* 608 */     MCD_OPC_FilterValue, 214, 1, 4, 0, // Skip to: 617
/* 613 */     MCD_OPC_Decode, 246, 5, 108, // Opcode: OC
/* 617 */     MCD_OPC_FilterValue, 215, 1, 4, 0, // Skip to: 626
/* 622 */     MCD_OPC_Decode, 129, 7, 108, // Opcode: XC
/* 626 */     MCD_OPC_FilterValue, 227, 1, 163, 2, // Skip to: 1306
/* 631 */     MCD_OPC_ExtractField, 0, 8,  // Inst{7-0} ...
/* 634 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 642
/* 638 */     MCD_OPC_Decode, 159, 5, 109, // Opcode: LTG
/* 642 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 650
/* 646 */     MCD_OPC_Decode, 210, 4, 109, // Opcode: LG
/* 650 */     MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 657
/* 654 */     MCD_OPC_Decode, 33, 110, // Opcode: AG
/* 657 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 665
/* 661 */     MCD_OPC_Decode, 166, 6, 110, // Opcode: SG
/* 665 */     MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 672
/* 669 */     MCD_OPC_Decode, 55, 110, // Opcode: ALG
/* 672 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 680
/* 676 */     MCD_OPC_Decode, 178, 6, 110, // Opcode: SLG
/* 680 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 688
/* 684 */     MCD_OPC_Decode, 198, 5, 110, // Opcode: MSG
/* 688 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 696
/* 692 */     MCD_OPC_Decode, 133, 4, 111, // Opcode: DSG
/* 696 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 704
/* 700 */     MCD_OPC_Decode, 151, 5, 109, // Opcode: LRVG
/* 704 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 712
/* 708 */     MCD_OPC_Decode, 154, 5, 112, // Opcode: LT
/* 712 */     MCD_OPC_FilterValue, 20, 4, 0, // Skip to: 720
/* 716 */     MCD_OPC_Decode, 214, 4, 109, // Opcode: LGF
/* 720 */     MCD_OPC_FilterValue, 21, 4, 0, // Skip to: 728
/* 724 */     MCD_OPC_Decode, 218, 4, 109, // Opcode: LGH
/* 728 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 736
/* 732 */     MCD_OPC_Decode, 239, 4, 109, // Opcode: LLGF
/* 736 */     MCD_OPC_FilterValue, 24, 3, 0, // Skip to: 743
/* 740 */     MCD_OPC_Decode, 34, 110, // Opcode: AGF
/* 743 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 751
/* 747 */     MCD_OPC_Decode, 167, 6, 110, // Opcode: SGF
/* 751 */     MCD_OPC_FilterValue, 26, 3, 0, // Skip to: 758
/* 755 */     MCD_OPC_Decode, 56, 110, // Opcode: ALGF
/* 758 */     MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 766
/* 762 */     MCD_OPC_Decode, 180, 6, 110, // Opcode: SLGF
/* 766 */     MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 774
/* 770 */     MCD_OPC_Decode, 199, 5, 110, // Opcode: MSGF
/* 774 */     MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 782
/* 778 */     MCD_OPC_Decode, 134, 4, 111, // Opcode: DSGF
/* 782 */     MCD_OPC_FilterValue, 30, 4, 0, // Skip to: 790
/* 786 */     MCD_OPC_Decode, 150, 5, 112, // Opcode: LRV
/* 790 */     MCD_OPC_FilterValue, 32, 4, 0, // Skip to: 798
/* 794 */     MCD_OPC_Decode, 153, 3, 109, // Opcode: CG
/* 798 */     MCD_OPC_FilterValue, 33, 4, 0, // Skip to: 806
/* 802 */     MCD_OPC_Decode, 188, 3, 109, // Opcode: CLG
/* 806 */     MCD_OPC_FilterValue, 36, 4, 0, // Skip to: 814
/* 810 */     MCD_OPC_Decode, 217, 6, 109, // Opcode: STG
/* 814 */     MCD_OPC_FilterValue, 47, 4, 0, // Skip to: 822
/* 818 */     MCD_OPC_Decode, 230, 6, 109, // Opcode: STRVG
/* 822 */     MCD_OPC_FilterValue, 48, 4, 0, // Skip to: 830
/* 826 */     MCD_OPC_Decode, 156, 3, 109, // Opcode: CGF
/* 830 */     MCD_OPC_FilterValue, 49, 4, 0, // Skip to: 838
/* 834 */     MCD_OPC_Decode, 191, 3, 109, // Opcode: CLGF
/* 838 */     MCD_OPC_FilterValue, 50, 4, 0, // Skip to: 846
/* 842 */     MCD_OPC_Decode, 160, 5, 109, // Opcode: LTGF
/* 846 */     MCD_OPC_FilterValue, 52, 4, 0, // Skip to: 854
/* 850 */     MCD_OPC_Decode, 160, 3, 109, // Opcode: CGH
/* 854 */     MCD_OPC_FilterValue, 54, 4, 0, // Skip to: 862
/* 858 */     MCD_OPC_Decode, 144, 6, 113, // Opcode: PFD
/* 862 */     MCD_OPC_FilterValue, 62, 4, 0, // Skip to: 870
/* 866 */     MCD_OPC_Decode, 229, 6, 112, // Opcode: STRV
/* 870 */     MCD_OPC_FilterValue, 80, 4, 0, // Skip to: 878
/* 874 */     MCD_OPC_Decode, 232, 6, 112, // Opcode: STY
/* 878 */     MCD_OPC_FilterValue, 81, 4, 0, // Skip to: 886
/* 882 */     MCD_OPC_Decode, 204, 5, 114, // Opcode: MSY
/* 886 */     MCD_OPC_FilterValue, 84, 4, 0, // Skip to: 894
/* 890 */     MCD_OPC_Decode, 244, 5, 114, // Opcode: NY
/* 894 */     MCD_OPC_FilterValue, 85, 4, 0, // Skip to: 902
/* 898 */     MCD_OPC_Decode, 215, 3, 112, // Opcode: CLY
/* 902 */     MCD_OPC_FilterValue, 86, 4, 0, // Skip to: 910
/* 906 */     MCD_OPC_Decode, 143, 6, 114, // Opcode: OY
/* 910 */     MCD_OPC_FilterValue, 87, 4, 0, // Skip to: 918
/* 914 */     MCD_OPC_Decode, 144, 7, 114, // Opcode: XY
/* 918 */     MCD_OPC_FilterValue, 88, 4, 0, // Skip to: 926
/* 922 */     MCD_OPC_Decode, 172, 5, 112, // Opcode: LY
/* 926 */     MCD_OPC_FilterValue, 89, 4, 0, // Skip to: 934
/* 930 */     MCD_OPC_Decode, 232, 3, 112, // Opcode: CY
/* 934 */     MCD_OPC_FilterValue, 90, 4, 0, // Skip to: 942
/* 938 */     MCD_OPC_Decode, 143, 1, 114, // Opcode: AY
/* 942 */     MCD_OPC_FilterValue, 91, 4, 0, // Skip to: 950
/* 946 */     MCD_OPC_Decode, 234, 6, 114, // Opcode: SY
/* 950 */     MCD_OPC_FilterValue, 94, 3, 0, // Skip to: 957
/* 954 */     MCD_OPC_Decode, 65, 114, // Opcode: ALY
/* 957 */     MCD_OPC_FilterValue, 95, 4, 0, // Skip to: 965
/* 961 */     MCD_OPC_Decode, 190, 6, 114, // Opcode: SLY
/* 965 */     MCD_OPC_FilterValue, 112, 4, 0, // Skip to: 973
/* 969 */     MCD_OPC_Decode, 223, 6, 112, // Opcode: STHY
/* 973 */     MCD_OPC_FilterValue, 113, 4, 0, // Skip to: 981
/* 977 */     MCD_OPC_Decode, 183, 4, 109, // Opcode: LAY
/* 981 */     MCD_OPC_FilterValue, 114, 4, 0, // Skip to: 989
/* 985 */     MCD_OPC_Decode, 211, 6, 112, // Opcode: STCY
/* 989 */     MCD_OPC_FilterValue, 115, 4, 0, // Skip to: 997
/* 993 */     MCD_OPC_Decode, 150, 4, 110, // Opcode: ICY
/* 997 */     MCD_OPC_FilterValue, 118, 4, 0, // Skip to: 1005
/* 1001 */    MCD_OPC_Decode, 184, 4, 112, // Opcode: LB
/* 1005 */    MCD_OPC_FilterValue, 119, 4, 0, // Skip to: 1013
/* 1009 */    MCD_OPC_Decode, 211, 4, 109, // Opcode: LGB
/* 1013 */    MCD_OPC_FilterValue, 120, 4, 0, // Skip to: 1021
/* 1017 */    MCD_OPC_Decode, 231, 4, 112, // Opcode: LHY
/* 1021 */    MCD_OPC_FilterValue, 121, 4, 0, // Skip to: 1029
/* 1025 */    MCD_OPC_Decode, 175, 3, 112, // Opcode: CHY
/* 1029 */    MCD_OPC_FilterValue, 122, 3, 0, // Skip to: 1036
/* 1033 */    MCD_OPC_Decode, 47, 114, // Opcode: AHY
/* 1036 */    MCD_OPC_FilterValue, 123, 4, 0, // Skip to: 1044
/* 1040 */    MCD_OPC_Decode, 172, 6, 114, // Opcode: SHY
/* 1044 */    MCD_OPC_FilterValue, 124, 4, 0, // Skip to: 1052
/* 1048 */    MCD_OPC_Decode, 189, 5, 114, // Opcode: MHY
/* 1052 */    MCD_OPC_FilterValue, 128, 1, 4, 0, // Skip to: 1061
/* 1057 */    MCD_OPC_Decode, 222, 5, 110, // Opcode: NG
/* 1061 */    MCD_OPC_FilterValue, 129, 1, 4, 0, // Skip to: 1070
/* 1066 */    MCD_OPC_Decode, 249, 5, 110, // Opcode: OG
/* 1070 */    MCD_OPC_FilterValue, 130, 1, 4, 0, // Skip to: 1079
/* 1075 */    MCD_OPC_Decode, 132, 7, 110, // Opcode: XG
/* 1079 */    MCD_OPC_FilterValue, 134, 1, 4, 0, // Skip to: 1088
/* 1084 */    MCD_OPC_Decode, 190, 5, 111, // Opcode: MLG
/* 1088 */    MCD_OPC_FilterValue, 135, 1, 4, 0, // Skip to: 1097
/* 1093 */    MCD_OPC_Decode, 130, 4, 111, // Opcode: DLG
/* 1097 */    MCD_OPC_FilterValue, 136, 1, 3, 0, // Skip to: 1105
/* 1102 */    MCD_OPC_Decode, 51, 110, // Opcode: ALCG
/* 1105 */    MCD_OPC_FilterValue, 137, 1, 4, 0, // Skip to: 1114
/* 1110 */    MCD_OPC_Decode, 175, 6, 110, // Opcode: SLBG
/* 1114 */    MCD_OPC_FilterValue, 144, 1, 4, 0, // Skip to: 1123
/* 1119 */    MCD_OPC_Decode, 237, 4, 109, // Opcode: LLGC
/* 1123 */    MCD_OPC_FilterValue, 145, 1, 4, 0, // Skip to: 1132
/* 1128 */    MCD_OPC_Decode, 242, 4, 109, // Opcode: LLGH
/* 1132 */    MCD_OPC_FilterValue, 148, 1, 4, 0, // Skip to: 1141
/* 1137 */    MCD_OPC_Decode, 232, 4, 112, // Opcode: LLC
/* 1141 */    MCD_OPC_FilterValue, 149, 1, 4, 0, // Skip to: 1150
/* 1146 */    MCD_OPC_Decode, 245, 4, 112, // Opcode: LLH
/* 1150 */    MCD_OPC_FilterValue, 151, 1, 4, 0, // Skip to: 1159
/* 1155 */    MCD_OPC_Decode, 129, 4, 111, // Opcode: DL
/* 1159 */    MCD_OPC_FilterValue, 152, 1, 3, 0, // Skip to: 1167
/* 1164 */    MCD_OPC_Decode, 50, 114, // Opcode: ALC
/* 1167 */    MCD_OPC_FilterValue, 153, 1, 4, 0, // Skip to: 1176
/* 1172 */    MCD_OPC_Decode, 174, 6, 114, // Opcode: SLB
/* 1176 */    MCD_OPC_FilterValue, 192, 1, 8, 0, // Skip to: 1189
/* 1181 */    MCD_OPC_CheckPredicate, 3, 39, 9, // Skip to: 3528
/* 1185 */    MCD_OPC_Decode, 185, 4, 115, // Opcode: LBH
/* 1189 */    MCD_OPC_FilterValue, 194, 1, 8, 0, // Skip to: 1202
/* 1194 */    MCD_OPC_CheckPredicate, 3, 26, 9, // Skip to: 3528
/* 1198 */    MCD_OPC_Decode, 233, 4, 112, // Opcode: LLCH
/* 1202 */    MCD_OPC_FilterValue, 195, 1, 8, 0, // Skip to: 1215
/* 1207 */    MCD_OPC_CheckPredicate, 3, 13, 9, // Skip to: 3528
/* 1211 */    MCD_OPC_Decode, 209, 6, 115, // Opcode: STCH
/* 1215 */    MCD_OPC_FilterValue, 196, 1, 8, 0, // Skip to: 1228
/* 1220 */    MCD_OPC_CheckPredicate, 3, 0, 9, // Skip to: 3528
/* 1224 */    MCD_OPC_Decode, 225, 4, 115, // Opcode: LHH
/* 1228 */    MCD_OPC_FilterValue, 198, 1, 8, 0, // Skip to: 1241
/* 1233 */    MCD_OPC_CheckPredicate, 3, 243, 8, // Skip to: 3528
/* 1237 */    MCD_OPC_Decode, 246, 4, 112, // Opcode: LLHH
/* 1241 */    MCD_OPC_FilterValue, 199, 1, 8, 0, // Skip to: 1254
/* 1246 */    MCD_OPC_CheckPredicate, 3, 230, 8, // Skip to: 3528
/* 1250 */    MCD_OPC_Decode, 220, 6, 115, // Opcode: STHH
/* 1254 */    MCD_OPC_FilterValue, 202, 1, 8, 0, // Skip to: 1267
/* 1259 */    MCD_OPC_CheckPredicate, 3, 217, 8, // Skip to: 3528
/* 1263 */    MCD_OPC_Decode, 209, 4, 115, // Opcode: LFH
/* 1267 */    MCD_OPC_FilterValue, 203, 1, 8, 0, // Skip to: 1280
/* 1272 */    MCD_OPC_CheckPredicate, 3, 204, 8, // Skip to: 3528
/* 1276 */    MCD_OPC_Decode, 216, 6, 115, // Opcode: STFH
/* 1280 */    MCD_OPC_FilterValue, 205, 1, 8, 0, // Skip to: 1293
/* 1285 */    MCD_OPC_CheckPredicate, 3, 191, 8, // Skip to: 3528
/* 1289 */    MCD_OPC_Decode, 170, 3, 115, // Opcode: CHF
/* 1293 */    MCD_OPC_FilterValue, 207, 1, 182, 8, // Skip to: 3528
/* 1298 */    MCD_OPC_CheckPredicate, 3, 178, 8, // Skip to: 3528
/* 1302 */    MCD_OPC_Decode, 202, 3, 115, // Opcode: CLHF
/* 1306 */    MCD_OPC_FilterValue, 229, 1, 75, 0, // Skip to: 1386
/* 1311 */    MCD_OPC_ExtractField, 32, 8,  // Inst{39-32} ...
/* 1314 */    MCD_OPC_FilterValue, 68, 4, 0, // Skip to: 1322
/* 1318 */    MCD_OPC_Decode, 209, 5, 116, // Opcode: MVHHI
/* 1322 */    MCD_OPC_FilterValue, 72, 4, 0, // Skip to: 1330
/* 1326 */    MCD_OPC_Decode, 208, 5, 116, // Opcode: MVGHI
/* 1330 */    MCD_OPC_FilterValue, 76, 4, 0, // Skip to: 1338
/* 1334 */    MCD_OPC_Decode, 210, 5, 116, // Opcode: MVHI
/* 1338 */    MCD_OPC_FilterValue, 84, 4, 0, // Skip to: 1346
/* 1342 */    MCD_OPC_Decode, 171, 3, 116, // Opcode: CHHSI
/* 1346 */    MCD_OPC_FilterValue, 85, 4, 0, // Skip to: 1354
/* 1350 */    MCD_OPC_Decode, 203, 3, 117, // Opcode: CLHHSI
/* 1354 */    MCD_OPC_FilterValue, 88, 4, 0, // Skip to: 1362
/* 1358 */    MCD_OPC_Decode, 163, 3, 116, // Opcode: CGHSI
/* 1362 */    MCD_OPC_FilterValue, 89, 4, 0, // Skip to: 1370
/* 1366 */    MCD_OPC_Decode, 196, 3, 117, // Opcode: CLGHSI
/* 1370 */    MCD_OPC_FilterValue, 92, 4, 0, // Skip to: 1378
/* 1374 */    MCD_OPC_Decode, 174, 3, 116, // Opcode: CHSI
/* 1378 */    MCD_OPC_FilterValue, 93, 98, 8, // Skip to: 3528
/* 1382 */    MCD_OPC_Decode, 184, 3, 117, // Opcode: CLFHSI
/* 1386 */    MCD_OPC_FilterValue, 235, 1, 54, 4, // Skip to: 2469
/* 1391 */    MCD_OPC_ExtractField, 0, 8,  // Inst{7-0} ...
/* 1394 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1402
/* 1398 */    MCD_OPC_Decode, 129, 5, 118, // Opcode: LMG
/* 1402 */    MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 1410
/* 1406 */    MCD_OPC_Decode, 198, 6, 119, // Opcode: SRAG
/* 1410 */    MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 1418
/* 1414 */    MCD_OPC_Decode, 202, 6, 119, // Opcode: SRLG
/* 1418 */    MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 1426
/* 1422 */    MCD_OPC_Decode, 186, 6, 119, // Opcode: SLLG
/* 1426 */    MCD_OPC_FilterValue, 20, 4, 0, // Skip to: 1434
/* 1430 */    MCD_OPC_Decode, 226, 3, 120, // Opcode: CSY
/* 1434 */    MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 1442
/* 1438 */    MCD_OPC_Decode, 156, 6, 119, // Opcode: RLLG
/* 1442 */    MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 1450
/* 1446 */    MCD_OPC_Decode, 155, 6, 121, // Opcode: RLL
/* 1450 */    MCD_OPC_FilterValue, 36, 4, 0, // Skip to: 1458
/* 1454 */    MCD_OPC_Decode, 224, 6, 118, // Opcode: STMG
/* 1458 */    MCD_OPC_FilterValue, 48, 4, 0, // Skip to: 1466
/* 1462 */    MCD_OPC_Decode, 225, 3, 122, // Opcode: CSG
/* 1466 */    MCD_OPC_FilterValue, 81, 4, 0, // Skip to: 1474
/* 1470 */    MCD_OPC_Decode, 255, 6, 123, // Opcode: TMY
/* 1474 */    MCD_OPC_FilterValue, 82, 4, 0, // Skip to: 1482
/* 1478 */    MCD_OPC_Decode, 212, 5, 123, // Opcode: MVIY
/* 1482 */    MCD_OPC_FilterValue, 84, 4, 0, // Skip to: 1490
/* 1486 */    MCD_OPC_Decode, 241, 5, 123, // Opcode: NIY
/* 1490 */    MCD_OPC_FilterValue, 85, 4, 0, // Skip to: 1498
/* 1494 */    MCD_OPC_Decode, 208, 3, 123, // Opcode: CLIY
/* 1498 */    MCD_OPC_FilterValue, 86, 4, 0, // Skip to: 1506
/* 1502 */    MCD_OPC_Decode, 140, 6, 123, // Opcode: OIY
/* 1506 */    MCD_OPC_FilterValue, 87, 4, 0, // Skip to: 1514
/* 1510 */    MCD_OPC_Decode, 141, 7, 123, // Opcode: XIY
/* 1514 */    MCD_OPC_FilterValue, 106, 3, 0, // Skip to: 1521
/* 1518 */    MCD_OPC_Decode, 68, 124, // Opcode: ASI
/* 1521 */    MCD_OPC_FilterValue, 122, 3, 0, // Skip to: 1528
/* 1525 */    MCD_OPC_Decode, 41, 124, // Opcode: AGSI
/* 1528 */    MCD_OPC_FilterValue, 220, 1, 8, 0, // Skip to: 1541
/* 1533 */    MCD_OPC_CheckPredicate, 2, 199, 7, // Skip to: 3528
/* 1537 */    MCD_OPC_Decode, 199, 6, 121, // Opcode: SRAK
/* 1541 */    MCD_OPC_FilterValue, 222, 1, 8, 0, // Skip to: 1554
/* 1546 */    MCD_OPC_CheckPredicate, 2, 186, 7, // Skip to: 3528
/* 1550 */    MCD_OPC_Decode, 203, 6, 121, // Opcode: SRLK
/* 1554 */    MCD_OPC_FilterValue, 223, 1, 8, 0, // Skip to: 1567
/* 1559 */    MCD_OPC_CheckPredicate, 2, 173, 7, // Skip to: 3528
/* 1563 */    MCD_OPC_Decode, 187, 6, 121, // Opcode: SLLK
/* 1567 */    MCD_OPC_FilterValue, 226, 1, 179, 0, // Skip to: 1751
/* 1572 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 1575 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1587
/* 1579 */    MCD_OPC_CheckPredicate, 1, 160, 0, // Skip to: 1743
/* 1583 */    MCD_OPC_Decode, 248, 2, 125, // Opcode: AsmOLOCG
/* 1587 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1599
/* 1591 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 1743
/* 1595 */    MCD_OPC_Decode, 177, 1, 125, // Opcode: AsmHLOCG
/* 1599 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1611
/* 1603 */    MCD_OPC_CheckPredicate, 1, 136, 0, // Skip to: 1743
/* 1607 */    MCD_OPC_Decode, 213, 2, 125, // Opcode: AsmNLELOCG
/* 1611 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 1623
/* 1615 */    MCD_OPC_CheckPredicate, 1, 124, 0, // Skip to: 1743
/* 1619 */    MCD_OPC_Decode, 172, 2, 125, // Opcode: AsmLLOCG
/* 1623 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 1635
/* 1627 */    MCD_OPC_CheckPredicate, 1, 112, 0, // Skip to: 1743
/* 1631 */    MCD_OPC_Decode, 195, 2, 125, // Opcode: AsmNHELOCG
/* 1635 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 1647
/* 1639 */    MCD_OPC_CheckPredicate, 1, 100, 0, // Skip to: 1743
/* 1643 */    MCD_OPC_Decode, 164, 2, 125, // Opcode: AsmLHLOCG
/* 1647 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 1659
/* 1651 */    MCD_OPC_CheckPredicate, 1, 88, 0, // Skip to: 1743
/* 1655 */    MCD_OPC_Decode, 185, 2, 125, // Opcode: AsmNELOCG
/* 1659 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1671
/* 1663 */    MCD_OPC_CheckPredicate, 1, 76, 0, // Skip to: 1743
/* 1667 */    MCD_OPC_Decode, 159, 1, 125, // Opcode: AsmELOCG
/* 1671 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1683
/* 1675 */    MCD_OPC_CheckPredicate, 1, 64, 0, // Skip to: 1743
/* 1679 */    MCD_OPC_Decode, 222, 2, 125, // Opcode: AsmNLHLOCG
/* 1683 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 1695
/* 1687 */    MCD_OPC_CheckPredicate, 1, 52, 0, // Skip to: 1743
/* 1691 */    MCD_OPC_Decode, 169, 1, 125, // Opcode: AsmHELOCG
/* 1695 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1707
/* 1699 */    MCD_OPC_CheckPredicate, 1, 40, 0, // Skip to: 1743
/* 1703 */    MCD_OPC_Decode, 230, 2, 125, // Opcode: AsmNLLOCG
/* 1707 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 1719
/* 1711 */    MCD_OPC_CheckPredicate, 1, 28, 0, // Skip to: 1743
/* 1715 */    MCD_OPC_Decode, 155, 2, 125, // Opcode: AsmLELOCG
/* 1719 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 1731
/* 1723 */    MCD_OPC_CheckPredicate, 1, 16, 0, // Skip to: 1743
/* 1727 */    MCD_OPC_Decode, 203, 2, 125, // Opcode: AsmNHLOCG
/* 1731 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1743
/* 1735 */    MCD_OPC_CheckPredicate, 1, 4, 0, // Skip to: 1743
/* 1739 */    MCD_OPC_Decode, 239, 2, 125, // Opcode: AsmNOLOCG
/* 1743 */    MCD_OPC_CheckPredicate, 1, 245, 6, // Skip to: 3528
/* 1747 */    MCD_OPC_Decode, 176, 2, 126, // Opcode: AsmLOCG
/* 1751 */    MCD_OPC_FilterValue, 227, 1, 180, 0, // Skip to: 1936
/* 1756 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 1759 */    MCD_OPC_FilterValue, 1, 8, 0, // Skip to: 1771
/* 1763 */    MCD_OPC_CheckPredicate, 1, 160, 0, // Skip to: 1927
/* 1767 */    MCD_OPC_Decode, 252, 2, 127, // Opcode: AsmOSTOCG
/* 1771 */    MCD_OPC_FilterValue, 2, 8, 0, // Skip to: 1783
/* 1775 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 1927
/* 1779 */    MCD_OPC_Decode, 181, 1, 127, // Opcode: AsmHSTOCG
/* 1783 */    MCD_OPC_FilterValue, 3, 8, 0, // Skip to: 1795
/* 1787 */    MCD_OPC_CheckPredicate, 1, 136, 0, // Skip to: 1927
/* 1791 */    MCD_OPC_Decode, 217, 2, 127, // Opcode: AsmNLESTOCG
/* 1795 */    MCD_OPC_FilterValue, 4, 8, 0, // Skip to: 1807
/* 1799 */    MCD_OPC_CheckPredicate, 1, 124, 0, // Skip to: 1927
/* 1803 */    MCD_OPC_Decode, 180, 2, 127, // Opcode: AsmLSTOCG
/* 1807 */    MCD_OPC_FilterValue, 5, 8, 0, // Skip to: 1819
/* 1811 */    MCD_OPC_CheckPredicate, 1, 112, 0, // Skip to: 1927
/* 1815 */    MCD_OPC_Decode, 199, 2, 127, // Opcode: AsmNHESTOCG
/* 1819 */    MCD_OPC_FilterValue, 6, 8, 0, // Skip to: 1831
/* 1823 */    MCD_OPC_CheckPredicate, 1, 100, 0, // Skip to: 1927
/* 1827 */    MCD_OPC_Decode, 168, 2, 127, // Opcode: AsmLHSTOCG
/* 1831 */    MCD_OPC_FilterValue, 7, 8, 0, // Skip to: 1843
/* 1835 */    MCD_OPC_CheckPredicate, 1, 88, 0, // Skip to: 1927
/* 1839 */    MCD_OPC_Decode, 189, 2, 127, // Opcode: AsmNESTOCG
/* 1843 */    MCD_OPC_FilterValue, 8, 8, 0, // Skip to: 1855
/* 1847 */    MCD_OPC_CheckPredicate, 1, 76, 0, // Skip to: 1927
/* 1851 */    MCD_OPC_Decode, 163, 1, 127, // Opcode: AsmESTOCG
/* 1855 */    MCD_OPC_FilterValue, 9, 8, 0, // Skip to: 1867
/* 1859 */    MCD_OPC_CheckPredicate, 1, 64, 0, // Skip to: 1927
/* 1863 */    MCD_OPC_Decode, 226, 2, 127, // Opcode: AsmNLHSTOCG
/* 1867 */    MCD_OPC_FilterValue, 10, 8, 0, // Skip to: 1879
/* 1871 */    MCD_OPC_CheckPredicate, 1, 52, 0, // Skip to: 1927
/* 1875 */    MCD_OPC_Decode, 173, 1, 127, // Opcode: AsmHESTOCG
/* 1879 */    MCD_OPC_FilterValue, 11, 8, 0, // Skip to: 1891
/* 1883 */    MCD_OPC_CheckPredicate, 1, 40, 0, // Skip to: 1927
/* 1887 */    MCD_OPC_Decode, 234, 2, 127, // Opcode: AsmNLSTOCG
/* 1891 */    MCD_OPC_FilterValue, 12, 8, 0, // Skip to: 1903
/* 1895 */    MCD_OPC_CheckPredicate, 1, 28, 0, // Skip to: 1927
/* 1899 */    MCD_OPC_Decode, 159, 2, 127, // Opcode: AsmLESTOCG
/* 1903 */    MCD_OPC_FilterValue, 13, 8, 0, // Skip to: 1915
/* 1907 */    MCD_OPC_CheckPredicate, 1, 16, 0, // Skip to: 1927
/* 1911 */    MCD_OPC_Decode, 207, 2, 127, // Opcode: AsmNHSTOCG
/* 1915 */    MCD_OPC_FilterValue, 14, 8, 0, // Skip to: 1927
/* 1919 */    MCD_OPC_CheckPredicate, 1, 4, 0, // Skip to: 1927
/* 1923 */    MCD_OPC_Decode, 243, 2, 127, // Opcode: AsmNOSTOCG
/* 1927 */    MCD_OPC_CheckPredicate, 1, 61, 6, // Skip to: 3528
/* 1931 */    MCD_OPC_Decode, 254, 2, 128, 1, // Opcode: AsmSTOCG
/* 1936 */    MCD_OPC_FilterValue, 228, 1, 8, 0, // Skip to: 1949
/* 1941 */    MCD_OPC_CheckPredicate, 4, 47, 6, // Skip to: 3528
/* 1945 */    MCD_OPC_Decode, 177, 4, 118, // Opcode: LANG
/* 1949 */    MCD_OPC_FilterValue, 230, 1, 8, 0, // Skip to: 1962
/* 1954 */    MCD_OPC_CheckPredicate, 4, 34, 6, // Skip to: 3528
/* 1958 */    MCD_OPC_Decode, 179, 4, 118, // Opcode: LAOG
/* 1962 */    MCD_OPC_FilterValue, 231, 1, 8, 0, // Skip to: 1975
/* 1967 */    MCD_OPC_CheckPredicate, 4, 21, 6, // Skip to: 3528
/* 1971 */    MCD_OPC_Decode, 182, 4, 118, // Opcode: LAXG
/* 1975 */    MCD_OPC_FilterValue, 232, 1, 8, 0, // Skip to: 1988
/* 1980 */    MCD_OPC_CheckPredicate, 4, 8, 6, // Skip to: 3528
/* 1984 */    MCD_OPC_Decode, 173, 4, 118, // Opcode: LAAG
/* 1988 */    MCD_OPC_FilterValue, 234, 1, 8, 0, // Skip to: 2001
/* 1993 */    MCD_OPC_CheckPredicate, 4, 251, 5, // Skip to: 3528
/* 1997 */    MCD_OPC_Decode, 175, 4, 118, // Opcode: LAALG
/* 2001 */    MCD_OPC_FilterValue, 242, 1, 194, 0, // Skip to: 2200
/* 2006 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 2009 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 2022
/* 2013 */    MCD_OPC_CheckPredicate, 1, 174, 0, // Skip to: 2191
/* 2017 */    MCD_OPC_Decode, 247, 2, 129, 1, // Opcode: AsmOLOC
/* 2022 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 2035
/* 2026 */    MCD_OPC_CheckPredicate, 1, 161, 0, // Skip to: 2191
/* 2030 */    MCD_OPC_Decode, 176, 1, 129, 1, // Opcode: AsmHLOC
/* 2035 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 2048
/* 2039 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 2191
/* 2043 */    MCD_OPC_Decode, 212, 2, 129, 1, // Opcode: AsmNLELOC
/* 2048 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 2061
/* 2052 */    MCD_OPC_CheckPredicate, 1, 135, 0, // Skip to: 2191
/* 2056 */    MCD_OPC_Decode, 171, 2, 129, 1, // Opcode: AsmLLOC
/* 2061 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 2074
/* 2065 */    MCD_OPC_CheckPredicate, 1, 122, 0, // Skip to: 2191
/* 2069 */    MCD_OPC_Decode, 194, 2, 129, 1, // Opcode: AsmNHELOC
/* 2074 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 2087
/* 2078 */    MCD_OPC_CheckPredicate, 1, 109, 0, // Skip to: 2191
/* 2082 */    MCD_OPC_Decode, 163, 2, 129, 1, // Opcode: AsmLHLOC
/* 2087 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 2100
/* 2091 */    MCD_OPC_CheckPredicate, 1, 96, 0, // Skip to: 2191
/* 2095 */    MCD_OPC_Decode, 184, 2, 129, 1, // Opcode: AsmNELOC
/* 2100 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 2113
/* 2104 */    MCD_OPC_CheckPredicate, 1, 83, 0, // Skip to: 2191
/* 2108 */    MCD_OPC_Decode, 158, 1, 129, 1, // Opcode: AsmELOC
/* 2113 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 2126
/* 2117 */    MCD_OPC_CheckPredicate, 1, 70, 0, // Skip to: 2191
/* 2121 */    MCD_OPC_Decode, 221, 2, 129, 1, // Opcode: AsmNLHLOC
/* 2126 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 2139
/* 2130 */    MCD_OPC_CheckPredicate, 1, 57, 0, // Skip to: 2191
/* 2134 */    MCD_OPC_Decode, 168, 1, 129, 1, // Opcode: AsmHELOC
/* 2139 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 2152
/* 2143 */    MCD_OPC_CheckPredicate, 1, 44, 0, // Skip to: 2191
/* 2147 */    MCD_OPC_Decode, 229, 2, 129, 1, // Opcode: AsmNLLOC
/* 2152 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 2165
/* 2156 */    MCD_OPC_CheckPredicate, 1, 31, 0, // Skip to: 2191
/* 2160 */    MCD_OPC_Decode, 154, 2, 129, 1, // Opcode: AsmLELOC
/* 2165 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 2178
/* 2169 */    MCD_OPC_CheckPredicate, 1, 18, 0, // Skip to: 2191
/* 2173 */    MCD_OPC_Decode, 202, 2, 129, 1, // Opcode: AsmNHLOC
/* 2178 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 2191
/* 2182 */    MCD_OPC_CheckPredicate, 1, 5, 0, // Skip to: 2191
/* 2186 */    MCD_OPC_Decode, 238, 2, 129, 1, // Opcode: AsmNOLOC
/* 2191 */    MCD_OPC_CheckPredicate, 1, 53, 5, // Skip to: 3528
/* 2195 */    MCD_OPC_Decode, 175, 2, 130, 1, // Opcode: AsmLOC
/* 2200 */    MCD_OPC_FilterValue, 243, 1, 194, 0, // Skip to: 2399
/* 2205 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 2208 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 2221
/* 2212 */    MCD_OPC_CheckPredicate, 1, 174, 0, // Skip to: 2390
/* 2216 */    MCD_OPC_Decode, 251, 2, 131, 1, // Opcode: AsmOSTOC
/* 2221 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 2234
/* 2225 */    MCD_OPC_CheckPredicate, 1, 161, 0, // Skip to: 2390
/* 2229 */    MCD_OPC_Decode, 180, 1, 131, 1, // Opcode: AsmHSTOC
/* 2234 */    MCD_OPC_FilterValue, 3, 9, 0, // Skip to: 2247
/* 2238 */    MCD_OPC_CheckPredicate, 1, 148, 0, // Skip to: 2390
/* 2242 */    MCD_OPC_Decode, 216, 2, 131, 1, // Opcode: AsmNLESTOC
/* 2247 */    MCD_OPC_FilterValue, 4, 9, 0, // Skip to: 2260
/* 2251 */    MCD_OPC_CheckPredicate, 1, 135, 0, // Skip to: 2390
/* 2255 */    MCD_OPC_Decode, 179, 2, 131, 1, // Opcode: AsmLSTOC
/* 2260 */    MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 2273
/* 2264 */    MCD_OPC_CheckPredicate, 1, 122, 0, // Skip to: 2390
/* 2268 */    MCD_OPC_Decode, 198, 2, 131, 1, // Opcode: AsmNHESTOC
/* 2273 */    MCD_OPC_FilterValue, 6, 9, 0, // Skip to: 2286
/* 2277 */    MCD_OPC_CheckPredicate, 1, 109, 0, // Skip to: 2390
/* 2281 */    MCD_OPC_Decode, 167, 2, 131, 1, // Opcode: AsmLHSTOC
/* 2286 */    MCD_OPC_FilterValue, 7, 9, 0, // Skip to: 2299
/* 2290 */    MCD_OPC_CheckPredicate, 1, 96, 0, // Skip to: 2390
/* 2294 */    MCD_OPC_Decode, 188, 2, 131, 1, // Opcode: AsmNESTOC
/* 2299 */    MCD_OPC_FilterValue, 8, 9, 0, // Skip to: 2312
/* 2303 */    MCD_OPC_CheckPredicate, 1, 83, 0, // Skip to: 2390
/* 2307 */    MCD_OPC_Decode, 162, 1, 131, 1, // Opcode: AsmESTOC
/* 2312 */    MCD_OPC_FilterValue, 9, 9, 0, // Skip to: 2325
/* 2316 */    MCD_OPC_CheckPredicate, 1, 70, 0, // Skip to: 2390
/* 2320 */    MCD_OPC_Decode, 225, 2, 131, 1, // Opcode: AsmNLHSTOC
/* 2325 */    MCD_OPC_FilterValue, 10, 9, 0, // Skip to: 2338
/* 2329 */    MCD_OPC_CheckPredicate, 1, 57, 0, // Skip to: 2390
/* 2333 */    MCD_OPC_Decode, 172, 1, 131, 1, // Opcode: AsmHESTOC
/* 2338 */    MCD_OPC_FilterValue, 11, 9, 0, // Skip to: 2351
/* 2342 */    MCD_OPC_CheckPredicate, 1, 44, 0, // Skip to: 2390
/* 2346 */    MCD_OPC_Decode, 233, 2, 131, 1, // Opcode: AsmNLSTOC
/* 2351 */    MCD_OPC_FilterValue, 12, 9, 0, // Skip to: 2364
/* 2355 */    MCD_OPC_CheckPredicate, 1, 31, 0, // Skip to: 2390
/* 2359 */    MCD_OPC_Decode, 158, 2, 131, 1, // Opcode: AsmLESTOC
/* 2364 */    MCD_OPC_FilterValue, 13, 9, 0, // Skip to: 2377
/* 2368 */    MCD_OPC_CheckPredicate, 1, 18, 0, // Skip to: 2390
/* 2372 */    MCD_OPC_Decode, 206, 2, 131, 1, // Opcode: AsmNHSTOC
/* 2377 */    MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 2390
/* 2381 */    MCD_OPC_CheckPredicate, 1, 5, 0, // Skip to: 2390
/* 2385 */    MCD_OPC_Decode, 242, 2, 131, 1, // Opcode: AsmNOSTOC
/* 2390 */    MCD_OPC_CheckPredicate, 1, 110, 4, // Skip to: 3528
/* 2394 */    MCD_OPC_Decode, 253, 2, 132, 1, // Opcode: AsmSTOC
/* 2399 */    MCD_OPC_FilterValue, 244, 1, 9, 0, // Skip to: 2413
/* 2404 */    MCD_OPC_CheckPredicate, 4, 96, 4, // Skip to: 3528
/* 2408 */    MCD_OPC_Decode, 176, 4, 133, 1, // Opcode: LAN
/* 2413 */    MCD_OPC_FilterValue, 246, 1, 9, 0, // Skip to: 2427
/* 2418 */    MCD_OPC_CheckPredicate, 4, 82, 4, // Skip to: 3528
/* 2422 */    MCD_OPC_Decode, 178, 4, 133, 1, // Opcode: LAO
/* 2427 */    MCD_OPC_FilterValue, 247, 1, 9, 0, // Skip to: 2441
/* 2432 */    MCD_OPC_CheckPredicate, 4, 68, 4, // Skip to: 3528
/* 2436 */    MCD_OPC_Decode, 181, 4, 133, 1, // Opcode: LAX
/* 2441 */    MCD_OPC_FilterValue, 248, 1, 9, 0, // Skip to: 2455
/* 2446 */    MCD_OPC_CheckPredicate, 4, 54, 4, // Skip to: 3528
/* 2450 */    MCD_OPC_Decode, 172, 4, 133, 1, // Opcode: LAA
/* 2455 */    MCD_OPC_FilterValue, 250, 1, 44, 4, // Skip to: 3528
/* 2460 */    MCD_OPC_CheckPredicate, 4, 40, 4, // Skip to: 3528
/* 2464 */    MCD_OPC_Decode, 174, 4, 133, 1, // Opcode: LAAL
/* 2469 */    MCD_OPC_FilterValue, 236, 1, 185, 2, // Skip to: 3171
/* 2474 */    MCD_OPC_ExtractField, 0, 8,  // Inst{7-0} ...
/* 2477 */    MCD_OPC_FilterValue, 81, 9, 0, // Skip to: 2490
/* 2481 */    MCD_OPC_CheckPredicate, 3, 19, 4, // Skip to: 3528
/* 2485 */    MCD_OPC_Decode, 151, 6, 134, 1, // Opcode: RISBLG
/* 2490 */    MCD_OPC_FilterValue, 84, 5, 0, // Skip to: 2499
/* 2494 */    MCD_OPC_Decode, 157, 6, 135, 1, // Opcode: RNSBG
/* 2499 */    MCD_OPC_FilterValue, 85, 5, 0, // Skip to: 2508
/* 2503 */    MCD_OPC_Decode, 146, 6, 135, 1, // Opcode: RISBG
/* 2508 */    MCD_OPC_FilterValue, 86, 5, 0, // Skip to: 2517
/* 2512 */    MCD_OPC_Decode, 158, 6, 135, 1, // Opcode: ROSBG
/* 2517 */    MCD_OPC_FilterValue, 87, 5, 0, // Skip to: 2526
/* 2521 */    MCD_OPC_Decode, 159, 6, 135, 1, // Opcode: RXSBG
/* 2526 */    MCD_OPC_FilterValue, 93, 9, 0, // Skip to: 2539
/* 2530 */    MCD_OPC_CheckPredicate, 3, 226, 3, // Skip to: 3528
/* 2534 */    MCD_OPC_Decode, 148, 6, 136, 1, // Opcode: RISBHG
/* 2539 */    MCD_OPC_FilterValue, 100, 69, 0, // Skip to: 2612
/* 2543 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2546 */    MCD_OPC_FilterValue, 0, 210, 3, // Skip to: 3528
/* 2550 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2553 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2562
/* 2557 */    MCD_OPC_Decode, 207, 1, 137, 1, // Opcode: AsmJHCGR
/* 2562 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2571
/* 2566 */    MCD_OPC_Decode, 239, 1, 137, 1, // Opcode: AsmJLCGR
/* 2571 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2580
/* 2575 */    MCD_OPC_Decode, 143, 2, 137, 1, // Opcode: AsmJLHCGR
/* 2580 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2589
/* 2584 */    MCD_OPC_Decode, 191, 1, 137, 1, // Opcode: AsmJECGR
/* 2589 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2598
/* 2593 */    MCD_OPC_Decode, 223, 1, 137, 1, // Opcode: AsmJHECGR
/* 2598 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2607
/* 2602 */    MCD_OPC_Decode, 255, 1, 137, 1, // Opcode: AsmJLECGR
/* 2607 */    MCD_OPC_Decode, 148, 1, 138, 1, // Opcode: AsmCGRJ
/* 2612 */    MCD_OPC_FilterValue, 101, 69, 0, // Skip to: 2685
/* 2616 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2619 */    MCD_OPC_FilterValue, 0, 137, 3, // Skip to: 3528
/* 2623 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2626 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2635
/* 2630 */    MCD_OPC_Decode, 210, 1, 137, 1, // Opcode: AsmJHCLGR
/* 2635 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2644
/* 2639 */    MCD_OPC_Decode, 242, 1, 137, 1, // Opcode: AsmJLCLGR
/* 2644 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2653
/* 2648 */    MCD_OPC_Decode, 146, 2, 137, 1, // Opcode: AsmJLHCLGR
/* 2653 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2662
/* 2657 */    MCD_OPC_Decode, 194, 1, 137, 1, // Opcode: AsmJECLGR
/* 2662 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2671
/* 2666 */    MCD_OPC_Decode, 226, 1, 137, 1, // Opcode: AsmJHECLGR
/* 2671 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2680
/* 2675 */    MCD_OPC_Decode, 130, 2, 137, 1, // Opcode: AsmJLECLGR
/* 2680 */    MCD_OPC_Decode, 151, 1, 138, 1, // Opcode: AsmCLGRJ
/* 2685 */    MCD_OPC_FilterValue, 118, 69, 0, // Skip to: 2758
/* 2689 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2692 */    MCD_OPC_FilterValue, 0, 64, 3, // Skip to: 3528
/* 2696 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2699 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2708
/* 2703 */    MCD_OPC_Decode, 213, 1, 139, 1, // Opcode: AsmJHCR
/* 2708 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2717
/* 2712 */    MCD_OPC_Decode, 245, 1, 139, 1, // Opcode: AsmJLCR
/* 2717 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2726
/* 2721 */    MCD_OPC_Decode, 149, 2, 139, 1, // Opcode: AsmJLHCR
/* 2726 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2735
/* 2730 */    MCD_OPC_Decode, 197, 1, 139, 1, // Opcode: AsmJECR
/* 2735 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2744
/* 2739 */    MCD_OPC_Decode, 229, 1, 139, 1, // Opcode: AsmJHECR
/* 2744 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2753
/* 2748 */    MCD_OPC_Decode, 133, 2, 139, 1, // Opcode: AsmJLECR
/* 2753 */    MCD_OPC_Decode, 154, 1, 140, 1, // Opcode: AsmCRJ
/* 2758 */    MCD_OPC_FilterValue, 119, 69, 0, // Skip to: 2831
/* 2762 */    MCD_OPC_ExtractField, 8, 4,  // Inst{11-8} ...
/* 2765 */    MCD_OPC_FilterValue, 0, 247, 2, // Skip to: 3528
/* 2769 */    MCD_OPC_ExtractField, 12, 4,  // Inst{15-12} ...
/* 2772 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2781
/* 2776 */    MCD_OPC_Decode, 212, 1, 139, 1, // Opcode: AsmJHCLR
/* 2781 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2790
/* 2785 */    MCD_OPC_Decode, 244, 1, 139, 1, // Opcode: AsmJLCLR
/* 2790 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2799
/* 2794 */    MCD_OPC_Decode, 148, 2, 139, 1, // Opcode: AsmJLHCLR
/* 2799 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2808
/* 2803 */    MCD_OPC_Decode, 196, 1, 139, 1, // Opcode: AsmJECLR
/* 2808 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2817
/* 2812 */    MCD_OPC_Decode, 228, 1, 139, 1, // Opcode: AsmJHECLR
/* 2817 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2826
/* 2821 */    MCD_OPC_Decode, 132, 2, 139, 1, // Opcode: AsmJLECLR
/* 2826 */    MCD_OPC_Decode, 153, 1, 140, 1, // Opcode: AsmCLRJ
/* 2831 */    MCD_OPC_FilterValue, 124, 62, 0, // Skip to: 2897
/* 2835 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 2838 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2847
/* 2842 */    MCD_OPC_Decode, 206, 1, 141, 1, // Opcode: AsmJHCGI
/* 2847 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2856
/* 2851 */    MCD_OPC_Decode, 238, 1, 141, 1, // Opcode: AsmJLCGI
/* 2856 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2865
/* 2860 */    MCD_OPC_Decode, 142, 2, 141, 1, // Opcode: AsmJLHCGI
/* 2865 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2874
/* 2869 */    MCD_OPC_Decode, 190, 1, 141, 1, // Opcode: AsmJECGI
/* 2874 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2883
/* 2878 */    MCD_OPC_Decode, 222, 1, 141, 1, // Opcode: AsmJHECGI
/* 2883 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2892
/* 2887 */    MCD_OPC_Decode, 254, 1, 141, 1, // Opcode: AsmJLECGI
/* 2892 */    MCD_OPC_Decode, 147, 1, 142, 1, // Opcode: AsmCGIJ
/* 2897 */    MCD_OPC_FilterValue, 125, 62, 0, // Skip to: 2963
/* 2901 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 2904 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2913
/* 2908 */    MCD_OPC_Decode, 209, 1, 143, 1, // Opcode: AsmJHCLGI
/* 2913 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2922
/* 2917 */    MCD_OPC_Decode, 241, 1, 143, 1, // Opcode: AsmJLCLGI
/* 2922 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2931
/* 2926 */    MCD_OPC_Decode, 145, 2, 143, 1, // Opcode: AsmJLHCLGI
/* 2931 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 2940
/* 2935 */    MCD_OPC_Decode, 193, 1, 143, 1, // Opcode: AsmJECLGI
/* 2940 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 2949
/* 2944 */    MCD_OPC_Decode, 225, 1, 143, 1, // Opcode: AsmJHECLGI
/* 2949 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 2958
/* 2953 */    MCD_OPC_Decode, 129, 2, 143, 1, // Opcode: AsmJLECLGI
/* 2958 */    MCD_OPC_Decode, 150, 1, 144, 1, // Opcode: AsmCLGIJ
/* 2963 */    MCD_OPC_FilterValue, 126, 62, 0, // Skip to: 3029
/* 2967 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 2970 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 2979
/* 2974 */    MCD_OPC_Decode, 208, 1, 145, 1, // Opcode: AsmJHCI
/* 2979 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 2988
/* 2983 */    MCD_OPC_Decode, 240, 1, 145, 1, // Opcode: AsmJLCI
/* 2988 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 2997
/* 2992 */    MCD_OPC_Decode, 144, 2, 145, 1, // Opcode: AsmJLHCI
/* 2997 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 3006
/* 3001 */    MCD_OPC_Decode, 192, 1, 145, 1, // Opcode: AsmJECI
/* 3006 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 3015
/* 3010 */    MCD_OPC_Decode, 224, 1, 145, 1, // Opcode: AsmJHECI
/* 3015 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 3024
/* 3019 */    MCD_OPC_Decode, 128, 2, 145, 1, // Opcode: AsmJLECI
/* 3024 */    MCD_OPC_Decode, 149, 1, 146, 1, // Opcode: AsmCIJ
/* 3029 */    MCD_OPC_FilterValue, 127, 62, 0, // Skip to: 3095
/* 3033 */    MCD_OPC_ExtractField, 32, 4,  // Inst{35-32} ...
/* 3036 */    MCD_OPC_FilterValue, 2, 5, 0, // Skip to: 3045
/* 3040 */    MCD_OPC_Decode, 211, 1, 147, 1, // Opcode: AsmJHCLI
/* 3045 */    MCD_OPC_FilterValue, 4, 5, 0, // Skip to: 3054
/* 3049 */    MCD_OPC_Decode, 243, 1, 147, 1, // Opcode: AsmJLCLI
/* 3054 */    MCD_OPC_FilterValue, 6, 5, 0, // Skip to: 3063
/* 3058 */    MCD_OPC_Decode, 147, 2, 147, 1, // Opcode: AsmJLHCLI
/* 3063 */    MCD_OPC_FilterValue, 8, 5, 0, // Skip to: 3072
/* 3067 */    MCD_OPC_Decode, 195, 1, 147, 1, // Opcode: AsmJECLI
/* 3072 */    MCD_OPC_FilterValue, 10, 5, 0, // Skip to: 3081
/* 3076 */    MCD_OPC_Decode, 227, 1, 147, 1, // Opcode: AsmJHECLI
/* 3081 */    MCD_OPC_FilterValue, 12, 5, 0, // Skip to: 3090
/* 3085 */    MCD_OPC_Decode, 131, 2, 147, 1, // Opcode: AsmJLECLI
/* 3090 */    MCD_OPC_Decode, 152, 1, 148, 1, // Opcode: AsmCLIJ
/* 3095 */    MCD_OPC_FilterValue, 216, 1, 14, 0, // Skip to: 3114
/* 3100 */    MCD_OPC_CheckPredicate, 2, 168, 1, // Skip to: 3528
/* 3104 */    MCD_OPC_CheckField, 8, 8, 0, 162, 1, // Skip to: 3528
/* 3110 */    MCD_OPC_Decode, 44, 149, 1, // Opcode: AHIK
/* 3114 */    MCD_OPC_FilterValue, 217, 1, 14, 0, // Skip to: 3133
/* 3119 */    MCD_OPC_CheckPredicate, 2, 149, 1, // Skip to: 3528
/* 3123 */    MCD_OPC_CheckField, 8, 8, 0, 143, 1, // Skip to: 3528
/* 3129 */    MCD_OPC_Decode, 38, 150, 1, // Opcode: AGHIK
/* 3133 */    MCD_OPC_FilterValue, 218, 1, 14, 0, // Skip to: 3152
/* 3138 */    MCD_OPC_CheckPredicate, 2, 130, 1, // Skip to: 3528
/* 3142 */    MCD_OPC_CheckField, 8, 8, 0, 124, 1, // Skip to: 3528
/* 3148 */    MCD_OPC_Decode, 62, 149, 1, // Opcode: ALHSIK
/* 3152 */    MCD_OPC_FilterValue, 219, 1, 115, 1, // Skip to: 3528
/* 3157 */    MCD_OPC_CheckPredicate, 2, 111, 1, // Skip to: 3528
/* 3161 */    MCD_OPC_CheckField, 8, 8, 0, 105, 1, // Skip to: 3528
/* 3167 */    MCD_OPC_Decode, 59, 150, 1, // Opcode: ALGHSIK
/* 3171 */    MCD_OPC_FilterValue, 237, 1, 96, 1, // Skip to: 3528
/* 3176 */    MCD_OPC_ExtractField, 0, 8,  // Inst{7-0} ...
/* 3179 */    MCD_OPC_FilterValue, 4, 11, 0, // Skip to: 3194
/* 3183 */    MCD_OPC_CheckField, 8, 8, 0, 83, 1, // Skip to: 3528
/* 3189 */    MCD_OPC_Decode, 195, 4, 151, 1, // Opcode: LDEB
/* 3194 */    MCD_OPC_FilterValue, 5, 11, 0, // Skip to: 3209
/* 3198 */    MCD_OPC_CheckField, 8, 8, 0, 68, 1, // Skip to: 3528
/* 3204 */    MCD_OPC_Decode, 167, 5, 152, 1, // Opcode: LXDB
/* 3209 */    MCD_OPC_FilterValue, 6, 11, 0, // Skip to: 3224
/* 3213 */    MCD_OPC_CheckField, 8, 8, 0, 53, 1, // Skip to: 3528
/* 3219 */    MCD_OPC_Decode, 169, 5, 152, 1, // Opcode: LXEB
/* 3224 */    MCD_OPC_FilterValue, 7, 11, 0, // Skip to: 3239
/* 3228 */    MCD_OPC_CheckField, 8, 8, 0, 38, 1, // Skip to: 3528
/* 3234 */    MCD_OPC_Decode, 216, 5, 153, 1, // Opcode: MXDB
/* 3239 */    MCD_OPC_FilterValue, 9, 11, 0, // Skip to: 3254
/* 3243 */    MCD_OPC_CheckField, 8, 8, 0, 23, 1, // Skip to: 3528
/* 3249 */    MCD_OPC_Decode, 142, 3, 154, 1, // Opcode: CEB
/* 3254 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 3268
/* 3258 */    MCD_OPC_CheckField, 8, 8, 0, 8, 1, // Skip to: 3528
/* 3264 */    MCD_OPC_Decode, 28, 155, 1, // Opcode: AEB
/* 3268 */    MCD_OPC_FilterValue, 11, 11, 0, // Skip to: 3283
/* 3272 */    MCD_OPC_CheckField, 8, 8, 0, 250, 0, // Skip to: 3528
/* 3278 */    MCD_OPC_Decode, 164, 6, 155, 1, // Opcode: SEB
/* 3283 */    MCD_OPC_FilterValue, 12, 11, 0, // Skip to: 3298
/* 3287 */    MCD_OPC_CheckField, 8, 8, 0, 235, 0, // Skip to: 3528
/* 3293 */    MCD_OPC_Decode, 182, 5, 156, 1, // Opcode: MDEB
/* 3298 */    MCD_OPC_FilterValue, 13, 11, 0, // Skip to: 3313
/* 3302 */    MCD_OPC_CheckField, 8, 8, 0, 220, 0, // Skip to: 3528
/* 3308 */    MCD_OPC_Decode, 255, 3, 155, 1, // Opcode: DEB
/* 3313 */    MCD_OPC_FilterValue, 14, 11, 0, // Skip to: 3328
/* 3317 */    MCD_OPC_CheckField, 8, 4, 0, 205, 0, // Skip to: 3528
/* 3323 */    MCD_OPC_Decode, 178, 5, 157, 1, // Opcode: MAEB
/* 3328 */    MCD_OPC_FilterValue, 15, 11, 0, // Skip to: 3343
/* 3332 */    MCD_OPC_CheckField, 8, 4, 0, 190, 0, // Skip to: 3528
/* 3338 */    MCD_OPC_Decode, 195, 5, 157, 1, // Opcode: MSEB
/* 3343 */    MCD_OPC_FilterValue, 20, 11, 0, // Skip to: 3358
/* 3347 */    MCD_OPC_CheckField, 8, 8, 0, 175, 0, // Skip to: 3528
/* 3353 */    MCD_OPC_Decode, 193, 6, 154, 1, // Opcode: SQEB
/* 3358 */    MCD_OPC_FilterValue, 21, 11, 0, // Skip to: 3373
/* 3362 */    MCD_OPC_CheckField, 8, 8, 0, 160, 0, // Skip to: 3528
/* 3368 */    MCD_OPC_Decode, 191, 6, 151, 1, // Opcode: SQDB
/* 3373 */    MCD_OPC_FilterValue, 23, 11, 0, // Skip to: 3388
/* 3377 */    MCD_OPC_CheckField, 8, 8, 0, 145, 0, // Skip to: 3528
/* 3383 */    MCD_OPC_Decode, 184, 5, 155, 1, // Opcode: MEEB
/* 3388 */    MCD_OPC_FilterValue, 25, 11, 0, // Skip to: 3403
/* 3392 */    MCD_OPC_CheckField, 8, 8, 0, 130, 0, // Skip to: 3528
/* 3398 */    MCD_OPC_Decode, 136, 3, 151, 1, // Opcode: CDB
/* 3403 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 3417
/* 3407 */    MCD_OPC_CheckField, 8, 8, 0, 115, 0, // Skip to: 3528
/* 3413 */    MCD_OPC_Decode, 23, 156, 1, // Opcode: ADB
/* 3417 */    MCD_OPC_FilterValue, 27, 11, 0, // Skip to: 3432
/* 3421 */    MCD_OPC_CheckField, 8, 8, 0, 101, 0, // Skip to: 3528
/* 3427 */    MCD_OPC_Decode, 162, 6, 156, 1, // Opcode: SDB
/* 3432 */    MCD_OPC_FilterValue, 28, 11, 0, // Skip to: 3447
/* 3436 */    MCD_OPC_CheckField, 8, 8, 0, 86, 0, // Skip to: 3528
/* 3442 */    MCD_OPC_Decode, 180, 5, 156, 1, // Opcode: MDB
/* 3447 */    MCD_OPC_FilterValue, 29, 11, 0, // Skip to: 3462
/* 3451 */    MCD_OPC_CheckField, 8, 8, 0, 71, 0, // Skip to: 3528
/* 3457 */    MCD_OPC_Decode, 253, 3, 156, 1, // Opcode: DDB
/* 3462 */    MCD_OPC_FilterValue, 30, 11, 0, // Skip to: 3477
/* 3466 */    MCD_OPC_CheckField, 8, 4, 0, 56, 0, // Skip to: 3528
/* 3472 */    MCD_OPC_Decode, 176, 5, 158, 1, // Opcode: MADB
/* 3477 */    MCD_OPC_FilterValue, 31, 11, 0, // Skip to: 3492
/* 3481 */    MCD_OPC_CheckField, 8, 4, 0, 41, 0, // Skip to: 3528
/* 3487 */    MCD_OPC_Decode, 193, 5, 158, 1, // Opcode: MSDB
/* 3492 */    MCD_OPC_FilterValue, 100, 5, 0, // Skip to: 3501
/* 3496 */    MCD_OPC_Decode, 208, 4, 159, 1, // Opcode: LEY
/* 3501 */    MCD_OPC_FilterValue, 101, 5, 0, // Skip to: 3510
/* 3505 */    MCD_OPC_Decode, 201, 4, 160, 1, // Opcode: LDY
/* 3510 */    MCD_OPC_FilterValue, 102, 5, 0, // Skip to: 3519
/* 3514 */    MCD_OPC_Decode, 215, 6, 159, 1, // Opcode: STEY
/* 3519 */    MCD_OPC_FilterValue, 103, 5, 0, // Skip to: 3528
/* 3523 */    MCD_OPC_Decode, 213, 6, 160, 1, // Opcode: STDY
/* 3528 */    MCD_OPC_Fail,
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
    return getbool((Bits & SystemZ_FeatureFPExtension));
  case 1:
    return getbool((Bits & SystemZ_FeatureLoadStoreOnCond));
  case 2:
    return getbool((Bits & SystemZ_FeatureDistinctOps));
  case 3:
    return getbool((Bits & SystemZ_FeatureHighWord));
  case 4:
    return getbool((Bits & SystemZ_FeatureInterlockedAccess1));
  }
}

#define DecodeToMCInst(fname,fieldname, InsnType) \
static DecodeStatus fname(DecodeStatus S, unsigned Idx, InsnType insn, MCInst *MI, \
                uint64_t Address, const void *Decoder) \
{ \
  InsnType tmp; \
  switch (Idx) { \
  default: \
  case 0: \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeADDR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 1: \
    tmp = fieldname(insn, 4, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 2: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeADDR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 3: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 4: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 5: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 6: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 7: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 8: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 9: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 10: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 11: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 12: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 13: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeBDAddr32Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 14: \
    tmp = fieldname(insn, 0, 16); \
    if (decodeBDAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 15: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 16: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 17: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 18: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 19: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 20: \
    tmp = fieldname(insn, 0, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 21: \
    tmp = fieldname(insn, 20, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 22: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 23: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 24: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 25: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 26: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 27: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 28: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 29: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 30: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (decodeAccessRegOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 31: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 32: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 33: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 34: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 35: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 36: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 37: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 38: \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 39: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 40: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 41: \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 42: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 43: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 44: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 45: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 46: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 47: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 48: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 49: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 50: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 51: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 52: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 53: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 54: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 55: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 56: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 57: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 58: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 59: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 60: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 61: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 62: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 63: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 64: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 65: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 66: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 67: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 68: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 69: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 70: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 71: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 72: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 73: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 74: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 75: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 76: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 77: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 78: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 79: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 80: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 81: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 82: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 83: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 84: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 85: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 86: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 87: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 88: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 89: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 90: \
    tmp = fieldname(insn, 4, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 91: \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 20, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeBDAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 92: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodePC32DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 93: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 94: \
    tmp = fieldname(insn, 0, 32); \
    if (decodePC32DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 95: \
    tmp = fieldname(insn, 36, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodePC32DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 96: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 97: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 98: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 99: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 100: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 101: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 102: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 103: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeU32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 104: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 105: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodePC32DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 106: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 107: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 32); \
    if (decodeS32ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 108: \
    tmp = fieldname(insn, 16, 24); \
    if (decodeBDLAddr64Disp12Len8Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeBDAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 109: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 110: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 111: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 112: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 113: \
    tmp = fieldname(insn, 36, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 114: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 115: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 116: \
    tmp = fieldname(insn, 16, 16); \
    if (decodeBDAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 117: \
    tmp = fieldname(insn, 16, 16); \
    if (decodeBDAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeU16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 118: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 119: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr32Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 120: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 121: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr32Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 122: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 123: \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 124: \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 8); \
    if (decodeS8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 125: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 126: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 127: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 128: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 129: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 130: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 131: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 132: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 133: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 24); \
    if (decodeBDAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 134: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 24, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU6ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 135: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 24, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU6ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 136: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGRH32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 24, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU6ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 137: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 138: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 139: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 140: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 141: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeS8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 142: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeS8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 143: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 144: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 145: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeS8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 146: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeS8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 147: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 148: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 8); \
    if (decodeU8ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (decodeU4ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodePC16DBLOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 149: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 150: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 32, 4); \
    if (DecodeGR64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 16); \
    if (decodeS16ImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 151: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 152: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 153: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP128BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 154: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 155: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 156: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 157: \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 158: \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 20); \
    if (decodeBDXAddr64Disp12Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 159: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP32BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 160: \
    tmp = fieldname(insn, 36, 4); \
    if (DecodeFP64BitRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 28); \
    if (decodeBDXAddr64Disp20Operand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  } \
} 

#define DecodeInstruction(fname, fieldname, decoder, InsnType) \
static DecodeStatus fname(const uint8_t DecodeTable[], MCInst *MI, \
           InsnType insn, uint64_t Address, const MCRegisterInfo *MRI, int feature) \
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

FieldFromInstruction(fieldFromInstruction, uint64_t)
DecodeToMCInst(decodeToMCInst, fieldFromInstruction, uint64_t)
DecodeInstruction(decodeInstruction, fieldFromInstruction, decodeToMCInst, uint64_t)
