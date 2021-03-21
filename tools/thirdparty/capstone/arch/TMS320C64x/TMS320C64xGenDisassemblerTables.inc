/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* * TMS320C64x Disassembler                                                  *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "../../MCInst.h"
#include "../../LEB128.h"

// Helper function for extracting fields from encoded instructions.
#define FieldFromInstruction(fname, InsnType) \
static InsnType fname(InsnType insn, unsigned startBit, \
                                     unsigned numBits) { \
    InsnType fieldMask; \
    if (numBits == sizeof(InsnType)*8) \
      fieldMask = (InsnType)(-1LL); \
    else \
      fieldMask = (((InsnType)1 << numBits) - 1) << startBit; \
    return (insn & fieldMask) >> startBit; \
}

static uint8_t DecoderTable32[] = {
/* 0 */       MCD_OPC_ExtractField, 2, 5,  // Inst{6-2} ...
/* 3 */       MCD_OPC_FilterValue, 0, 199, 0, // Skip to: 206
/* 7 */       MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 10 */      MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 30
/* 14 */      MCD_OPC_CheckField, 17, 11, 0, 153, 8, // Skip to: 2221
/* 20 */      MCD_OPC_CheckField, 12, 1, 0, 147, 8, // Skip to: 2221
/* 26 */      MCD_OPC_Decode, 162, 1, 0, // Opcode: NOP_n
/* 30 */      MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 38
/* 34 */      MCD_OPC_Decode, 140, 1, 1, // Opcode: MPYH_m4_rrr
/* 38 */      MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 46
/* 42 */      MCD_OPC_Decode, 219, 1, 1, // Opcode: SMPYH_m4_rrr
/* 46 */      MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 54
/* 50 */      MCD_OPC_Decode, 136, 1, 1, // Opcode: MPYHSU_m4_rrr
/* 54 */      MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 62
/* 58 */      MCD_OPC_Decode, 138, 1, 1, // Opcode: MPYHUS_m4_rrr
/* 62 */      MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 70
/* 66 */      MCD_OPC_Decode, 139, 1, 1, // Opcode: MPYHU_m4_rrr
/* 70 */      MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 78
/* 74 */      MCD_OPC_Decode, 134, 1, 1, // Opcode: MPYHL_m4_rrr
/* 78 */      MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 86
/* 82 */      MCD_OPC_Decode, 218, 1, 1, // Opcode: SMPYHL_m4_rrr
/* 86 */      MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 94
/* 90 */      MCD_OPC_Decode, 135, 1, 1, // Opcode: MPYHSLU_m4_rrr
/* 94 */      MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 102
/* 98 */      MCD_OPC_Decode, 137, 1, 1, // Opcode: MPYHULS_m4_rrr
/* 102 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 110
/* 106 */     MCD_OPC_Decode, 133, 1, 1, // Opcode: MPYHLU_m4_rrr
/* 110 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 118
/* 114 */     MCD_OPC_Decode, 142, 1, 1, // Opcode: MPYLH_m4_rrr
/* 118 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 126
/* 122 */     MCD_OPC_Decode, 220, 1, 1, // Opcode: SMPYLH_m4_rrr
/* 126 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 134
/* 130 */     MCD_OPC_Decode, 145, 1, 1, // Opcode: MPYLSHU_m4_rrr
/* 134 */     MCD_OPC_FilterValue, 21, 4, 0, // Skip to: 142
/* 138 */     MCD_OPC_Decode, 146, 1, 1, // Opcode: MPYLUHS_m4_rrr
/* 142 */     MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 150
/* 146 */     MCD_OPC_Decode, 141, 1, 1, // Opcode: MPYLHU_m4_rrr
/* 150 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 158
/* 154 */     MCD_OPC_Decode, 153, 1, 2, // Opcode: MPY_m4_irr
/* 158 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 166
/* 162 */     MCD_OPC_Decode, 154, 1, 1, // Opcode: MPY_m4_rrr
/* 166 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 174
/* 170 */     MCD_OPC_Decode, 221, 1, 1, // Opcode: SMPY_m4_rrr
/* 174 */     MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 182
/* 178 */     MCD_OPC_Decode, 149, 1, 1, // Opcode: MPYSU_m4_rrr
/* 182 */     MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 190
/* 186 */     MCD_OPC_Decode, 151, 1, 1, // Opcode: MPYUS_m4_rrr
/* 190 */     MCD_OPC_FilterValue, 30, 4, 0, // Skip to: 198
/* 194 */     MCD_OPC_Decode, 148, 1, 2, // Opcode: MPYSU_m4_irr
/* 198 */     MCD_OPC_FilterValue, 31, 227, 7, // Skip to: 2221
/* 202 */     MCD_OPC_Decode, 152, 1, 1, // Opcode: MPYU_m4_rrr
/* 206 */     MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 219
/* 210 */     MCD_OPC_CheckField, 8, 1, 0, 213, 7, // Skip to: 2221
/* 216 */     MCD_OPC_Decode, 116, 3, // Opcode: LDHU_d5_mr
/* 219 */     MCD_OPC_FilterValue, 2, 18, 0, // Skip to: 241
/* 223 */     MCD_OPC_ExtractField, 7, 1,  // Inst{7} ...
/* 226 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 233
/* 230 */     MCD_OPC_Decode, 102, 4, // Opcode: EXTU_s15_riir
/* 233 */     MCD_OPC_FilterValue, 1, 192, 7, // Skip to: 2221
/* 237 */     MCD_OPC_Decode, 192, 1, 4, // Opcode: SET_s15_riir
/* 241 */     MCD_OPC_FilterValue, 3, 3, 0, // Skip to: 248
/* 245 */     MCD_OPC_Decode, 117, 5, // Opcode: LDHU_d6_mr
/* 248 */     MCD_OPC_FilterValue, 4, 3, 0, // Skip to: 255
/* 252 */     MCD_OPC_Decode, 68, 6, // Opcode: B_s5_i
/* 255 */     MCD_OPC_FilterValue, 5, 9, 0, // Skip to: 268
/* 259 */     MCD_OPC_CheckField, 8, 1, 0, 164, 7, // Skip to: 2221
/* 265 */     MCD_OPC_Decode, 111, 3, // Opcode: LDBU_d5_mr
/* 268 */     MCD_OPC_FilterValue, 6, 157, 0, // Skip to: 429
/* 272 */     MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 275 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 283
/* 279 */     MCD_OPC_Decode, 171, 1, 1, // Opcode: PACK2_l1_rrr_x2
/* 283 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 291
/* 287 */     MCD_OPC_Decode, 242, 1, 1, // Opcode: SUB2_l1_rrr_x2
/* 291 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 299
/* 295 */     MCD_OPC_Decode, 176, 1, 1, // Opcode: PACKHL2_l1_rrr_x2
/* 299 */     MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 306
/* 303 */     MCD_OPC_Decode, 45, 7, // Opcode: ADD_l1_ipp
/* 306 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 314
/* 310 */     MCD_OPC_Decode, 130, 2, 7, // Opcode: SUB_l1_ipp
/* 314 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 322
/* 318 */     MCD_OPC_Decode, 228, 1, 7, // Opcode: SSUB_l1_ipp
/* 322 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 330
/* 326 */     MCD_OPC_Decode, 186, 1, 7, // Opcode: SADD_l1_ipp
/* 330 */     MCD_OPC_FilterValue, 14, 9, 0, // Skip to: 343
/* 334 */     MCD_OPC_CheckField, 13, 5, 0, 89, 7, // Skip to: 2221
/* 340 */     MCD_OPC_Decode, 23, 8, // Opcode: ABS_l1_pp
/* 343 */     MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 357
/* 347 */     MCD_OPC_CheckField, 13, 5, 0, 76, 7, // Skip to: 2221
/* 353 */     MCD_OPC_Decode, 191, 1, 9, // Opcode: SAT_l1_pr
/* 357 */     MCD_OPC_FilterValue, 17, 3, 0, // Skip to: 364
/* 361 */     MCD_OPC_Decode, 82, 10, // Opcode: CMPGT_l1_ipr
/* 364 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 372
/* 368 */     MCD_OPC_Decode, 129, 1, 1, // Opcode: MINU4_l1_rrr_x2
/* 372 */     MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 379
/* 376 */     MCD_OPC_Decode, 106, 11, // Opcode: GMPGTU_l1_ipr
/* 379 */     MCD_OPC_FilterValue, 20, 3, 0, // Skip to: 386
/* 383 */     MCD_OPC_Decode, 76, 10, // Opcode: CMPEQ_l1_ipr
/* 386 */     MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 393
/* 390 */     MCD_OPC_Decode, 90, 10, // Opcode: CMPLT_l1_ipr
/* 393 */     MCD_OPC_FilterValue, 23, 3, 0, // Skip to: 400
/* 397 */     MCD_OPC_Decode, 86, 11, // Opcode: CMPLTU_l1_ipr
/* 400 */     MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 414
/* 404 */     MCD_OPC_CheckField, 13, 5, 0, 19, 7, // Skip to: 2221
/* 410 */     MCD_OPC_Decode, 163, 1, 12, // Opcode: NORM_l1_pr
/* 414 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 422
/* 418 */     MCD_OPC_Decode, 178, 1, 1, // Opcode: PACKL4_l1_rrr_x2
/* 422 */     MCD_OPC_FilterValue, 31, 3, 7, // Skip to: 2221
/* 426 */     MCD_OPC_Decode, 53, 1, // Opcode: ANDN_l1_rrr_x2
/* 429 */     MCD_OPC_FilterValue, 7, 3, 0, // Skip to: 436
/* 433 */     MCD_OPC_Decode, 112, 5, // Opcode: LDBU_d6_mr
/* 436 */     MCD_OPC_FilterValue, 8, 222, 0, // Skip to: 662
/* 440 */     MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 443 */     MCD_OPC_FilterValue, 0, 17, 0, // Skip to: 464
/* 447 */     MCD_OPC_ExtractField, 12, 1,  // Inst{12} ...
/* 450 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 457
/* 454 */     MCD_OPC_Decode, 67, 13, // Opcode: BPOS_s8_ir
/* 457 */     MCD_OPC_FilterValue, 1, 224, 6, // Skip to: 2221
/* 461 */     MCD_OPC_Decode, 63, 13, // Opcode: BDEC_s8_ir
/* 464 */     MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 477
/* 468 */     MCD_OPC_CheckField, 12, 1, 0, 211, 6, // Skip to: 2221
/* 474 */     MCD_OPC_Decode, 66, 14, // Opcode: BNOP_s9_ii
/* 477 */     MCD_OPC_FilterValue, 3, 3, 0, // Skip to: 484
/* 481 */     MCD_OPC_Decode, 50, 2, // Opcode: ADD_s1_irr
/* 484 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 492
/* 488 */     MCD_OPC_Decode, 177, 1, 1, // Opcode: PACKHL2_s1_rrr
/* 492 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 500
/* 496 */     MCD_OPC_Decode, 148, 2, 2, // Opcode: XOR_s1_irr
/* 500 */     MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 514
/* 504 */     MCD_OPC_CheckField, 13, 5, 0, 175, 6, // Skip to: 2221
/* 510 */     MCD_OPC_Decode, 156, 1, 15, // Opcode: MVC_s1_rr2
/* 514 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 522
/* 518 */     MCD_OPC_Decode, 180, 1, 1, // Opcode: PACKLH2_s1_rrr
/* 522 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 530
/* 526 */     MCD_OPC_Decode, 199, 1, 16, // Opcode: SHL_s1_rip
/* 530 */     MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 537
/* 534 */     MCD_OPC_Decode, 80, 1, // Opcode: CMPGT2_s1_rrr
/* 537 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 545
/* 541 */     MCD_OPC_Decode, 136, 2, 2, // Opcode: SUB_s1_irr
/* 545 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 553
/* 549 */     MCD_OPC_Decode, 203, 1, 17, // Opcode: SHR2_s1_rir
/* 553 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 561
/* 557 */     MCD_OPC_Decode, 169, 1, 2, // Opcode: OR_s1_irr
/* 561 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 568
/* 565 */     MCD_OPC_Decode, 75, 1, // Opcode: CMPEQ4_s1_rrr
/* 568 */     MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 575
/* 572 */     MCD_OPC_Decode, 59, 2, // Opcode: AND_s1_irr
/* 575 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 583
/* 579 */     MCD_OPC_Decode, 190, 1, 1, // Opcode: SADD_s1_rrr
/* 583 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 591
/* 587 */     MCD_OPC_Decode, 224, 1, 17, // Opcode: SSHL_s1_rir
/* 591 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 599
/* 595 */     MCD_OPC_Decode, 209, 1, 18, // Opcode: SHRU_s1_pip
/* 599 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 607
/* 603 */     MCD_OPC_Decode, 211, 1, 17, // Opcode: SHRU_s1_rir
/* 607 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 615
/* 611 */     MCD_OPC_Decode, 197, 1, 19, // Opcode: SHL_s1_pip
/* 615 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 623
/* 619 */     MCD_OPC_Decode, 200, 1, 20, // Opcode: SHL_s1_rir
/* 623 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 631
/* 627 */     MCD_OPC_Decode, 213, 1, 18, // Opcode: SHR_s1_pip
/* 631 */     MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 639
/* 635 */     MCD_OPC_Decode, 215, 1, 17, // Opcode: SHR_s1_rir
/* 639 */     MCD_OPC_FilterValue, 30, 42, 6, // Skip to: 2221
/* 643 */     MCD_OPC_ExtractField, 13, 5,  // Inst{17-13} ...
/* 646 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 654
/* 650 */     MCD_OPC_Decode, 143, 2, 21, // Opcode: UNPKLU4_s14_rr
/* 654 */     MCD_OPC_FilterValue, 3, 27, 6, // Skip to: 2221
/* 658 */     MCD_OPC_Decode, 141, 2, 21, // Opcode: UNPKHU4_s14_rr
/* 662 */     MCD_OPC_FilterValue, 9, 17, 0, // Skip to: 683
/* 666 */     MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 669 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 676
/* 673 */     MCD_OPC_Decode, 113, 3, // Opcode: LDB_d5_mr
/* 676 */     MCD_OPC_FilterValue, 1, 5, 6, // Skip to: 2221
/* 680 */     MCD_OPC_Decode, 120, 22, // Opcode: LDNDW_d8_mp
/* 683 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 691
/* 687 */     MCD_OPC_Decode, 159, 1, 23, // Opcode: MVKL_s12_ir
/* 691 */     MCD_OPC_FilterValue, 11, 3, 0, // Skip to: 698
/* 695 */     MCD_OPC_Decode, 114, 5, // Opcode: LDB_d6_mr
/* 698 */     MCD_OPC_FilterValue, 12, 194, 0, // Skip to: 896
/* 702 */     MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 705 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 713
/* 709 */     MCD_OPC_Decode, 130, 1, 24, // Opcode: MPY2_m1_rrp
/* 713 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 720
/* 717 */     MCD_OPC_Decode, 100, 1, // Opcode: DOTPSU4_m1_rrr
/* 720 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 728
/* 724 */     MCD_OPC_Decode, 150, 1, 24, // Opcode: MPYU4_m1_rrp
/* 728 */     MCD_OPC_FilterValue, 3, 3, 0, // Skip to: 735
/* 732 */     MCD_OPC_Decode, 101, 1, // Opcode: DOTPU4_m1_rrr
/* 735 */     MCD_OPC_FilterValue, 6, 3, 0, // Skip to: 742
/* 739 */     MCD_OPC_Decode, 96, 1, // Opcode: DOTP2_m1_rrr
/* 742 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 750
/* 746 */     MCD_OPC_Decode, 143, 1, 1, // Opcode: MPYLIR_m1_rrr
/* 750 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 758
/* 754 */     MCD_OPC_Decode, 131, 1, 1, // Opcode: MPYHIR_m1_rrr
/* 758 */     MCD_OPC_FilterValue, 9, 3, 0, // Skip to: 765
/* 762 */     MCD_OPC_Decode, 62, 1, // Opcode: AVGU4_m1_rrr
/* 765 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 773
/* 769 */     MCD_OPC_Decode, 132, 1, 24, // Opcode: MPYHI_m1_rrp
/* 773 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 781
/* 777 */     MCD_OPC_Decode, 227, 1, 1, // Opcode: SSHVR_m1_rrr
/* 781 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 789
/* 785 */     MCD_OPC_Decode, 226, 1, 1, // Opcode: SSHVL_m1_rrr
/* 789 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 797
/* 793 */     MCD_OPC_Decode, 181, 1, 17, // Opcode: ROTL_m1_rir
/* 797 */     MCD_OPC_FilterValue, 16, 3, 0, // Skip to: 804
/* 801 */     MCD_OPC_Decode, 52, 1, // Opcode: ANDN_d2_rrr
/* 804 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 812
/* 808 */     MCD_OPC_Decode, 166, 1, 1, // Opcode: OR_d2_rrr
/* 812 */     MCD_OPC_FilterValue, 18, 3, 0, // Skip to: 819
/* 816 */     MCD_OPC_Decode, 25, 1, // Opcode: ADD2_d2_rrr
/* 819 */     MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 826
/* 823 */     MCD_OPC_Decode, 56, 1, // Opcode: AND_d2_rrr
/* 826 */     MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 833
/* 830 */     MCD_OPC_Decode, 44, 1, // Opcode: ADD_d2_rrr
/* 833 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 841
/* 837 */     MCD_OPC_Decode, 129, 2, 1, // Opcode: SUB_d2_rrr
/* 841 */     MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 849
/* 845 */     MCD_OPC_Decode, 145, 2, 1, // Opcode: XOR_d2_rrr
/* 849 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 857
/* 853 */     MCD_OPC_Decode, 183, 1, 1, // Opcode: SADD2_s4_rrr
/* 857 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 865
/* 861 */     MCD_OPC_Decode, 222, 1, 1, // Opcode: SPACK2_s4_rrr
/* 865 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 873
/* 869 */     MCD_OPC_Decode, 223, 1, 1, // Opcode: SPACKU4_s4_rrr
/* 873 */     MCD_OPC_FilterValue, 27, 3, 0, // Skip to: 880
/* 877 */     MCD_OPC_Decode, 54, 1, // Opcode: ANDN_s4_rrr
/* 880 */     MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 888
/* 884 */     MCD_OPC_Decode, 208, 1, 1, // Opcode: SHRU2_s4_rrr
/* 888 */     MCD_OPC_FilterValue, 29, 49, 5, // Skip to: 2221
/* 892 */     MCD_OPC_Decode, 206, 1, 1, // Opcode: SHRMB_s4_rrr
/* 896 */     MCD_OPC_FilterValue, 13, 18, 0, // Skip to: 918
/* 900 */     MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 903 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 911
/* 907 */     MCD_OPC_Decode, 232, 1, 3, // Opcode: STB_d5_rm
/* 911 */     MCD_OPC_FilterValue, 1, 26, 5, // Skip to: 2221
/* 915 */     MCD_OPC_Decode, 121, 3, // Opcode: LDNW_d5_mr
/* 918 */     MCD_OPC_FilterValue, 14, 98, 0, // Skip to: 1020
/* 922 */     MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 925 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 932
/* 929 */     MCD_OPC_Decode, 26, 1, // Opcode: ADD2_l1_rrr_x2
/* 932 */     MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 939
/* 936 */     MCD_OPC_Decode, 47, 25, // Opcode: ADD_l1_rpp
/* 939 */     MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 946
/* 943 */     MCD_OPC_Decode, 39, 25, // Opcode: ADDU_l1_rpp
/* 946 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 954
/* 950 */     MCD_OPC_Decode, 188, 1, 25, // Opcode: SADD_l1_rpp
/* 954 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 962
/* 958 */     MCD_OPC_Decode, 128, 1, 1, // Opcode: MIN2_l1_rrr_x2
/* 962 */     MCD_OPC_FilterValue, 17, 3, 0, // Skip to: 969
/* 966 */     MCD_OPC_Decode, 84, 26, // Opcode: CMPGT_l1_rpr
/* 969 */     MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 976
/* 973 */     MCD_OPC_Decode, 108, 26, // Opcode: GMPGTU_l1_rpr
/* 976 */     MCD_OPC_FilterValue, 20, 3, 0, // Skip to: 983
/* 980 */     MCD_OPC_Decode, 78, 26, // Opcode: CMPEQ_l1_rpr
/* 983 */     MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 990
/* 987 */     MCD_OPC_Decode, 92, 26, // Opcode: CMPLT_l1_rpr
/* 990 */     MCD_OPC_FilterValue, 23, 3, 0, // Skip to: 997
/* 994 */     MCD_OPC_Decode, 88, 26, // Opcode: CMPLTU_l1_rpr
/* 997 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1005
/* 1001 */    MCD_OPC_Decode, 195, 1, 1, // Opcode: SHLMB_l1_rrr_x2
/* 1005 */    MCD_OPC_FilterValue, 25, 3, 0, // Skip to: 1012
/* 1009 */    MCD_OPC_Decode, 28, 1, // Opcode: ADD4_l1_rrr_x2
/* 1012 */    MCD_OPC_FilterValue, 26, 181, 4, // Skip to: 2221
/* 1016 */    MCD_OPC_Decode, 175, 1, 1, // Opcode: PACKH4_l1_rrr_x2
/* 1020 */    MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 1028
/* 1024 */    MCD_OPC_Decode, 233, 1, 5, // Opcode: STB_d6_rm
/* 1028 */    MCD_OPC_FilterValue, 16, 151, 0, // Skip to: 1183
/* 1032 */    MCD_OPC_ExtractField, 7, 6,  // Inst{12-7} ...
/* 1035 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1049
/* 1039 */    MCD_OPC_CheckField, 18, 5, 0, 152, 4, // Skip to: 2221
/* 1045 */    MCD_OPC_Decode, 160, 1, 27, // Opcode: MVK_d1_rr
/* 1049 */    MCD_OPC_FilterValue, 16, 3, 0, // Skip to: 1056
/* 1053 */    MCD_OPC_Decode, 42, 28, // Opcode: ADD_d1_rrr
/* 1056 */    MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 1064
/* 1060 */    MCD_OPC_Decode, 128, 2, 28, // Opcode: SUB_d1_rrr
/* 1064 */    MCD_OPC_FilterValue, 18, 3, 0, // Skip to: 1071
/* 1068 */    MCD_OPC_Decode, 41, 29, // Opcode: ADD_d1_rir
/* 1071 */    MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 1079
/* 1075 */    MCD_OPC_Decode, 255, 1, 29, // Opcode: SUB_d1_rir
/* 1079 */    MCD_OPC_FilterValue, 48, 3, 0, // Skip to: 1086
/* 1083 */    MCD_OPC_Decode, 30, 28, // Opcode: ADDAB_d1_rrr
/* 1086 */    MCD_OPC_FilterValue, 49, 4, 0, // Skip to: 1094
/* 1090 */    MCD_OPC_Decode, 247, 1, 28, // Opcode: SUBAB_d1_rrr
/* 1094 */    MCD_OPC_FilterValue, 50, 3, 0, // Skip to: 1101
/* 1098 */    MCD_OPC_Decode, 29, 29, // Opcode: ADDAB_d1_rir
/* 1101 */    MCD_OPC_FilterValue, 51, 4, 0, // Skip to: 1109
/* 1105 */    MCD_OPC_Decode, 246, 1, 29, // Opcode: SUBAB_d1_rir
/* 1109 */    MCD_OPC_FilterValue, 52, 3, 0, // Skip to: 1116
/* 1113 */    MCD_OPC_Decode, 34, 28, // Opcode: ADDAH_d1_rrr
/* 1116 */    MCD_OPC_FilterValue, 53, 4, 0, // Skip to: 1124
/* 1120 */    MCD_OPC_Decode, 249, 1, 28, // Opcode: SUBAH_d1_rrr
/* 1124 */    MCD_OPC_FilterValue, 54, 3, 0, // Skip to: 1131
/* 1128 */    MCD_OPC_Decode, 33, 29, // Opcode: ADDAH_d1_rir
/* 1131 */    MCD_OPC_FilterValue, 55, 4, 0, // Skip to: 1139
/* 1135 */    MCD_OPC_Decode, 248, 1, 29, // Opcode: SUBAH_d1_rir
/* 1139 */    MCD_OPC_FilterValue, 56, 3, 0, // Skip to: 1146
/* 1143 */    MCD_OPC_Decode, 36, 28, // Opcode: ADDAW_d1_rrr
/* 1146 */    MCD_OPC_FilterValue, 57, 4, 0, // Skip to: 1154
/* 1150 */    MCD_OPC_Decode, 251, 1, 28, // Opcode: SUBAW_d1_rrr
/* 1154 */    MCD_OPC_FilterValue, 58, 3, 0, // Skip to: 1161
/* 1158 */    MCD_OPC_Decode, 35, 29, // Opcode: ADDAW_d1_rir
/* 1161 */    MCD_OPC_FilterValue, 59, 4, 0, // Skip to: 1169
/* 1165 */    MCD_OPC_Decode, 250, 1, 29, // Opcode: SUBAW_d1_rir
/* 1169 */    MCD_OPC_FilterValue, 60, 3, 0, // Skip to: 1176
/* 1173 */    MCD_OPC_Decode, 32, 28, // Opcode: ADDAD_d1_rrr
/* 1176 */    MCD_OPC_FilterValue, 61, 17, 4, // Skip to: 2221
/* 1180 */    MCD_OPC_Decode, 31, 29, // Opcode: ADDAD_d1_rir
/* 1183 */    MCD_OPC_FilterValue, 17, 18, 0, // Skip to: 1205
/* 1187 */    MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 1190 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1197
/* 1194 */    MCD_OPC_Decode, 118, 3, // Opcode: LDH_d5_mr
/* 1197 */    MCD_OPC_FilterValue, 1, 252, 3, // Skip to: 2221
/* 1201 */    MCD_OPC_Decode, 234, 1, 30, // Opcode: STDW_d7_pm
/* 1205 */    MCD_OPC_FilterValue, 18, 17, 0, // Skip to: 1226
/* 1209 */    MCD_OPC_ExtractField, 7, 1,  // Inst{7} ...
/* 1212 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1219
/* 1216 */    MCD_OPC_Decode, 104, 4, // Opcode: EXT_s15_riir
/* 1219 */    MCD_OPC_FilterValue, 1, 230, 3, // Skip to: 2221
/* 1223 */    MCD_OPC_Decode, 72, 4, // Opcode: CLR_s15_riir
/* 1226 */    MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 1233
/* 1230 */    MCD_OPC_Decode, 119, 5, // Opcode: LDH_d6_mr
/* 1233 */    MCD_OPC_FilterValue, 20, 3, 0, // Skip to: 1240
/* 1237 */    MCD_OPC_Decode, 38, 23, // Opcode: ADDK_s2_ir
/* 1240 */    MCD_OPC_FilterValue, 21, 19, 0, // Skip to: 1263
/* 1244 */    MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 1247 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1255
/* 1251 */    MCD_OPC_Decode, 235, 1, 3, // Opcode: STH_d5_rm
/* 1255 */    MCD_OPC_FilterValue, 1, 194, 3, // Skip to: 2221
/* 1259 */    MCD_OPC_Decode, 238, 1, 3, // Opcode: STNW_d5_rm
/* 1263 */    MCD_OPC_FilterValue, 22, 191, 0, // Skip to: 1458
/* 1267 */    MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 1270 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1277
/* 1274 */    MCD_OPC_Decode, 46, 2, // Opcode: ADD_l1_irr
/* 1277 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1285
/* 1281 */    MCD_OPC_Decode, 131, 2, 2, // Opcode: SUB_l1_irr
/* 1285 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 1293
/* 1289 */    MCD_OPC_Decode, 229, 1, 2, // Opcode: SSUB_l1_irr
/* 1293 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1301
/* 1297 */    MCD_OPC_Decode, 187, 1, 2, // Opcode: SADD_l1_irr
/* 1301 */    MCD_OPC_FilterValue, 6, 49, 0, // Skip to: 1354
/* 1305 */    MCD_OPC_ExtractField, 13, 5,  // Inst{17-13} ...
/* 1308 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1315
/* 1312 */    MCD_OPC_Decode, 24, 21, // Opcode: ABS_l1_rr
/* 1315 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1323
/* 1319 */    MCD_OPC_Decode, 139, 2, 21, // Opcode: SWAP4_l2_rr
/* 1323 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 1331
/* 1327 */    MCD_OPC_Decode, 142, 2, 21, // Opcode: UNPKLU4_l2_rr
/* 1331 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 1339
/* 1335 */    MCD_OPC_Decode, 140, 2, 21, // Opcode: UNPKHU4_l2_rr
/* 1339 */    MCD_OPC_FilterValue, 4, 3, 0, // Skip to: 1346
/* 1343 */    MCD_OPC_Decode, 22, 21, // Opcode: ABS2_l2_rr
/* 1346 */    MCD_OPC_FilterValue, 5, 103, 3, // Skip to: 2221
/* 1350 */    MCD_OPC_Decode, 161, 1, 31, // Opcode: MVK_l2_ir
/* 1354 */    MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 1362
/* 1358 */    MCD_OPC_Decode, 173, 1, 1, // Opcode: PACKH2_l1_rrr_x2
/* 1362 */    MCD_OPC_FilterValue, 16, 3, 0, // Skip to: 1369
/* 1366 */    MCD_OPC_Decode, 126, 1, // Opcode: MAX2_l1_rrr_x2
/* 1369 */    MCD_OPC_FilterValue, 17, 3, 0, // Skip to: 1376
/* 1373 */    MCD_OPC_Decode, 83, 2, // Opcode: CMPGT_l1_irr
/* 1376 */    MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 1383
/* 1380 */    MCD_OPC_Decode, 107, 17, // Opcode: GMPGTU_l1_irr
/* 1383 */    MCD_OPC_FilterValue, 20, 3, 0, // Skip to: 1390
/* 1387 */    MCD_OPC_Decode, 77, 2, // Opcode: CMPEQ_l1_irr
/* 1390 */    MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 1397
/* 1394 */    MCD_OPC_Decode, 91, 2, // Opcode: CMPLT_l1_irr
/* 1397 */    MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 1405
/* 1401 */    MCD_OPC_Decode, 245, 1, 1, // Opcode: SUBABS4_l1_rrr_x2
/* 1405 */    MCD_OPC_FilterValue, 23, 3, 0, // Skip to: 1412
/* 1409 */    MCD_OPC_Decode, 87, 17, // Opcode: CMPLTU_l1_irr
/* 1412 */    MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1420
/* 1416 */    MCD_OPC_Decode, 205, 1, 1, // Opcode: SHRMB_l1_rrr_x2
/* 1420 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 1428
/* 1424 */    MCD_OPC_Decode, 244, 1, 1, // Opcode: SUB4_l1_rrr_x2
/* 1428 */    MCD_OPC_FilterValue, 26, 3, 0, // Skip to: 1435
/* 1432 */    MCD_OPC_Decode, 124, 2, // Opcode: LMBD_l1_irr
/* 1435 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 1443
/* 1439 */    MCD_OPC_Decode, 146, 2, 2, // Opcode: XOR_l1_irr
/* 1443 */    MCD_OPC_FilterValue, 30, 3, 0, // Skip to: 1450
/* 1447 */    MCD_OPC_Decode, 57, 2, // Opcode: AND_l1_irr
/* 1450 */    MCD_OPC_FilterValue, 31, 255, 2, // Skip to: 2221
/* 1454 */    MCD_OPC_Decode, 167, 1, 2, // Opcode: OR_l1_irr
/* 1458 */    MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 1466
/* 1462 */    MCD_OPC_Decode, 236, 1, 5, // Opcode: STH_d6_rm
/* 1466 */    MCD_OPC_FilterValue, 24, 6, 1, // Skip to: 1732
/* 1470 */    MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 1473 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1480
/* 1477 */    MCD_OPC_Decode, 27, 1, // Opcode: ADD2_s1_rrr
/* 1480 */    MCD_OPC_FilterValue, 1, 19, 0, // Skip to: 1503
/* 1484 */    MCD_OPC_ExtractField, 12, 16,  // Inst{27-12} ...
/* 1487 */    MCD_OPC_FilterValue, 128, 3, 3, 0, // Skip to: 1495
/* 1492 */    MCD_OPC_Decode, 70, 32, // Opcode: B_s7_irp
/* 1495 */    MCD_OPC_FilterValue, 192, 3, 209, 2, // Skip to: 2221
/* 1500 */    MCD_OPC_Decode, 71, 32, // Opcode: B_s7_nrp
/* 1503 */    MCD_OPC_FilterValue, 2, 9, 0, // Skip to: 1516
/* 1507 */    MCD_OPC_CheckField, 12, 1, 0, 196, 2, // Skip to: 2221
/* 1513 */    MCD_OPC_Decode, 37, 33, // Opcode: ADDKPC_s3_iir
/* 1516 */    MCD_OPC_FilterValue, 3, 3, 0, // Skip to: 1523
/* 1520 */    MCD_OPC_Decode, 51, 1, // Opcode: ADD_s1_rrr
/* 1523 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1531
/* 1527 */    MCD_OPC_Decode, 174, 1, 1, // Opcode: PACKH2_s1_rrr
/* 1531 */    MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 1539
/* 1535 */    MCD_OPC_Decode, 149, 2, 1, // Opcode: XOR_s1_rrr
/* 1539 */    MCD_OPC_FilterValue, 6, 29, 0, // Skip to: 1572
/* 1543 */    MCD_OPC_ExtractField, 23, 5,  // Inst{27-23} ...
/* 1546 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 1559
/* 1550 */    MCD_OPC_CheckField, 13, 5, 0, 153, 2, // Skip to: 2221
/* 1556 */    MCD_OPC_Decode, 69, 34, // Opcode: B_s6_r
/* 1559 */    MCD_OPC_FilterValue, 1, 146, 2, // Skip to: 2221
/* 1563 */    MCD_OPC_CheckField, 16, 2, 0, 140, 2, // Skip to: 2221
/* 1569 */    MCD_OPC_Decode, 65, 35, // Opcode: BNOP_s10_ri
/* 1572 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 1586
/* 1576 */    MCD_OPC_CheckField, 13, 5, 0, 127, 2, // Skip to: 2221
/* 1582 */    MCD_OPC_Decode, 155, 1, 36, // Opcode: MVC_s1_rr
/* 1586 */    MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 1594
/* 1590 */    MCD_OPC_Decode, 243, 1, 1, // Opcode: SUB2_s1_rrr
/* 1594 */    MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 1602
/* 1598 */    MCD_OPC_Decode, 201, 1, 37, // Opcode: SHL_s1_rrp
/* 1602 */    MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 1609
/* 1606 */    MCD_OPC_Decode, 81, 1, // Opcode: CMPGTU4_s1_rrr
/* 1609 */    MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 1617
/* 1613 */    MCD_OPC_Decode, 137, 2, 1, // Opcode: SUB_s1_rrr
/* 1617 */    MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 1625
/* 1621 */    MCD_OPC_Decode, 207, 1, 17, // Opcode: SHRU2_s1_rir
/* 1625 */    MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 1633
/* 1629 */    MCD_OPC_Decode, 170, 1, 1, // Opcode: OR_s1_rrr
/* 1633 */    MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 1640
/* 1637 */    MCD_OPC_Decode, 74, 1, // Opcode: CMPEQ2_s1_rrr
/* 1640 */    MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 1647
/* 1644 */    MCD_OPC_Decode, 60, 1, // Opcode: AND_s1_rrr
/* 1647 */    MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 1655
/* 1651 */    MCD_OPC_Decode, 225, 1, 1, // Opcode: SSHL_s1_rrr
/* 1655 */    MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 1663
/* 1659 */    MCD_OPC_Decode, 210, 1, 38, // Opcode: SHRU_s1_prp
/* 1663 */    MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 1671
/* 1667 */    MCD_OPC_Decode, 212, 1, 1, // Opcode: SHRU_s1_rrr
/* 1671 */    MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 1678
/* 1675 */    MCD_OPC_Decode, 103, 1, // Opcode: EXTU_s1_rrr
/* 1678 */    MCD_OPC_FilterValue, 23, 3, 0, // Skip to: 1685
/* 1682 */    MCD_OPC_Decode, 105, 1, // Opcode: EXT_s1_rrr
/* 1685 */    MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1693
/* 1689 */    MCD_OPC_Decode, 198, 1, 25, // Opcode: SHL_s1_prp
/* 1693 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 1701
/* 1697 */    MCD_OPC_Decode, 202, 1, 39, // Opcode: SHL_s1_rrr
/* 1701 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 1709
/* 1705 */    MCD_OPC_Decode, 214, 1, 38, // Opcode: SHR_s1_prp
/* 1709 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 1717
/* 1713 */    MCD_OPC_Decode, 216, 1, 1, // Opcode: SHR_s1_rrr
/* 1717 */    MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 1725
/* 1721 */    MCD_OPC_Decode, 193, 1, 1, // Opcode: SET_s1_rrr
/* 1725 */    MCD_OPC_FilterValue, 31, 236, 1, // Skip to: 2221
/* 1729 */    MCD_OPC_Decode, 73, 1, // Opcode: CLR_s1_rrr
/* 1732 */    MCD_OPC_FilterValue, 25, 17, 0, // Skip to: 1753
/* 1736 */    MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 1739 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1746
/* 1743 */    MCD_OPC_Decode, 122, 3, // Opcode: LDW_d5_mr
/* 1746 */    MCD_OPC_FilterValue, 1, 215, 1, // Skip to: 2221
/* 1750 */    MCD_OPC_Decode, 115, 30, // Opcode: LDDW_d7_mp
/* 1753 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 1761
/* 1757 */    MCD_OPC_Decode, 158, 1, 23, // Opcode: MVKLH_s12_ir
/* 1761 */    MCD_OPC_FilterValue, 27, 3, 0, // Skip to: 1768
/* 1765 */    MCD_OPC_Decode, 123, 5, // Opcode: LDW_d6_mr
/* 1768 */    MCD_OPC_FilterValue, 28, 216, 0, // Skip to: 1988
/* 1772 */    MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 1775 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1783
/* 1779 */    MCD_OPC_Decode, 217, 1, 24, // Opcode: SMPY2_m1_rrp
/* 1783 */    MCD_OPC_FilterValue, 1, 49, 0, // Skip to: 1836
/* 1787 */    MCD_OPC_ExtractField, 13, 5,  // Inst{17-13} ...
/* 1790 */    MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1798
/* 1794 */    MCD_OPC_Decode, 151, 2, 21, // Opcode: XPND4_m2_rr
/* 1798 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 1806
/* 1802 */    MCD_OPC_Decode, 150, 2, 21, // Opcode: XPND2_m2_rr
/* 1806 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 1814
/* 1810 */    MCD_OPC_Decode, 157, 1, 21, // Opcode: MVD_m2_rr
/* 1814 */    MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 1822
/* 1818 */    MCD_OPC_Decode, 194, 1, 21, // Opcode: SHFL_m2_rr
/* 1822 */    MCD_OPC_FilterValue, 29, 3, 0, // Skip to: 1829
/* 1826 */    MCD_OPC_Decode, 94, 21, // Opcode: DEAL_m2_rr
/* 1829 */    MCD_OPC_FilterValue, 30, 132, 1, // Skip to: 2221
/* 1833 */    MCD_OPC_Decode, 64, 21, // Opcode: BITC4_m2_rr
/* 1836 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 1844
/* 1840 */    MCD_OPC_Decode, 147, 1, 24, // Opcode: MPYSU4_m1_rrp
/* 1844 */    MCD_OPC_FilterValue, 3, 3, 0, // Skip to: 1851
/* 1848 */    MCD_OPC_Decode, 98, 1, // Opcode: DOTPNRSU2_m1_rrr
/* 1851 */    MCD_OPC_FilterValue, 4, 3, 0, // Skip to: 1858
/* 1855 */    MCD_OPC_Decode, 97, 1, // Opcode: DOTPN2_m1_rrr
/* 1858 */    MCD_OPC_FilterValue, 5, 3, 0, // Skip to: 1865
/* 1862 */    MCD_OPC_Decode, 95, 24, // Opcode: DOTP2_m1_rrp
/* 1865 */    MCD_OPC_FilterValue, 6, 3, 0, // Skip to: 1872
/* 1869 */    MCD_OPC_Decode, 99, 1, // Opcode: DOTPRSU2_m1_rrr
/* 1872 */    MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 1879
/* 1876 */    MCD_OPC_Decode, 110, 1, // Opcode: GMPY4_m1_rrr
/* 1879 */    MCD_OPC_FilterValue, 9, 3, 0, // Skip to: 1886
/* 1883 */    MCD_OPC_Decode, 61, 1, // Opcode: AVG2_m1_rrr
/* 1886 */    MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 1894
/* 1890 */    MCD_OPC_Decode, 144, 1, 24, // Opcode: MPYLI_m1_rrp
/* 1894 */    MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 1902
/* 1898 */    MCD_OPC_Decode, 182, 1, 1, // Opcode: ROTL_m1_rrr
/* 1902 */    MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 1910
/* 1906 */    MCD_OPC_Decode, 165, 1, 2, // Opcode: OR_d2_rir
/* 1910 */    MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 1918
/* 1914 */    MCD_OPC_Decode, 241, 1, 1, // Opcode: SUB2_d2_rrr
/* 1918 */    MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 1925
/* 1922 */    MCD_OPC_Decode, 55, 2, // Opcode: AND_d2_rir
/* 1925 */    MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 1932
/* 1929 */    MCD_OPC_Decode, 43, 2, // Opcode: ADD_d2_rir
/* 1932 */    MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 1940
/* 1936 */    MCD_OPC_Decode, 144, 2, 2, // Opcode: XOR_d2_rir
/* 1940 */    MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1948
/* 1944 */    MCD_OPC_Decode, 185, 1, 1, // Opcode: SADDUS2_s4_rrr
/* 1948 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 1956
/* 1952 */    MCD_OPC_Decode, 184, 1, 1, // Opcode: SADDU4_s4_rrr
/* 1956 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 1964
/* 1960 */    MCD_OPC_Decode, 138, 2, 1, // Opcode: SUB_s4_rrr
/* 1964 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 1972
/* 1968 */    MCD_OPC_Decode, 204, 1, 1, // Opcode: SHR2_s4_rrr
/* 1972 */    MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 1980
/* 1976 */    MCD_OPC_Decode, 196, 1, 1, // Opcode: SHLMB_s4_rrr
/* 1980 */    MCD_OPC_FilterValue, 31, 237, 0, // Skip to: 2221
/* 1984 */    MCD_OPC_Decode, 172, 1, 1, // Opcode: PACK2_s4_rrr
/* 1988 */    MCD_OPC_FilterValue, 29, 19, 0, // Skip to: 2011
/* 1992 */    MCD_OPC_ExtractField, 8, 1,  // Inst{8} ...
/* 1995 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2003
/* 1999 */    MCD_OPC_Decode, 239, 1, 3, // Opcode: STW_d5_rm
/* 2003 */    MCD_OPC_FilterValue, 1, 214, 0, // Skip to: 2221
/* 2007 */    MCD_OPC_Decode, 237, 1, 22, // Opcode: STNDW_d8_pm
/* 2011 */    MCD_OPC_FilterValue, 30, 198, 0, // Skip to: 2213
/* 2015 */    MCD_OPC_ExtractField, 7, 5,  // Inst{11-7} ...
/* 2018 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 2025
/* 2022 */    MCD_OPC_Decode, 49, 1, // Opcode: ADD_l1_rrr_x2
/* 2025 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 2033
/* 2029 */    MCD_OPC_Decode, 135, 2, 1, // Opcode: SUB_l1_rrr_x2
/* 2033 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 2041
/* 2037 */    MCD_OPC_Decode, 231, 1, 1, // Opcode: SSUB_l1_rrr_x2
/* 2041 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 2049
/* 2045 */    MCD_OPC_Decode, 189, 1, 1, // Opcode: SADD_l1_rrr_x2
/* 2049 */    MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 2057
/* 2053 */    MCD_OPC_Decode, 134, 2, 39, // Opcode: SUB_l1_rrr_x1
/* 2057 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 2065
/* 2061 */    MCD_OPC_Decode, 179, 1, 1, // Opcode: PACKLH2_l1_rrr_x2
/* 2065 */    MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 2073
/* 2069 */    MCD_OPC_Decode, 230, 1, 39, // Opcode: SSUB_l1_rrr_x1
/* 2073 */    MCD_OPC_FilterValue, 8, 3, 0, // Skip to: 2080
/* 2077 */    MCD_OPC_Decode, 48, 24, // Opcode: ADD_l1_rrp_x2
/* 2080 */    MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 2088
/* 2084 */    MCD_OPC_Decode, 133, 2, 24, // Opcode: SUB_l1_rrp_x2
/* 2088 */    MCD_OPC_FilterValue, 10, 3, 0, // Skip to: 2095
/* 2092 */    MCD_OPC_Decode, 40, 24, // Opcode: ADDU_l1_rrp_x2
/* 2095 */    MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 2103
/* 2099 */    MCD_OPC_Decode, 254, 1, 24, // Opcode: SUBU_l1_rrp_x2
/* 2103 */    MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 2111
/* 2107 */    MCD_OPC_Decode, 132, 2, 37, // Opcode: SUB_l1_rrp_x1
/* 2111 */    MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 2119
/* 2115 */    MCD_OPC_Decode, 253, 1, 37, // Opcode: SUBU_l1_rrp_x1
/* 2119 */    MCD_OPC_FilterValue, 16, 3, 0, // Skip to: 2126
/* 2123 */    MCD_OPC_Decode, 127, 1, // Opcode: MAXU4_l1_rrr_x2
/* 2126 */    MCD_OPC_FilterValue, 17, 3, 0, // Skip to: 2133
/* 2130 */    MCD_OPC_Decode, 85, 1, // Opcode: CMPGT_l1_rrr_x2
/* 2133 */    MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 2141
/* 2137 */    MCD_OPC_Decode, 252, 1, 1, // Opcode: SUBC_l1_rrr_x2
/* 2141 */    MCD_OPC_FilterValue, 19, 3, 0, // Skip to: 2148
/* 2145 */    MCD_OPC_Decode, 109, 1, // Opcode: GMPGTU_l1_rrr_x2
/* 2148 */    MCD_OPC_FilterValue, 20, 3, 0, // Skip to: 2155
/* 2152 */    MCD_OPC_Decode, 79, 1, // Opcode: CMPEQ_l1_rrr_x2
/* 2155 */    MCD_OPC_FilterValue, 21, 3, 0, // Skip to: 2162
/* 2159 */    MCD_OPC_Decode, 93, 1, // Opcode: CMPLT_l1_rrr_x2
/* 2162 */    MCD_OPC_FilterValue, 23, 3, 0, // Skip to: 2169
/* 2166 */    MCD_OPC_Decode, 89, 1, // Opcode: CMPLTU_l1_rrr_x2
/* 2169 */    MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 2183
/* 2173 */    MCD_OPC_CheckField, 13, 5, 0, 42, 0, // Skip to: 2221
/* 2179 */    MCD_OPC_Decode, 164, 1, 21, // Opcode: NORM_l1_rr
/* 2183 */    MCD_OPC_FilterValue, 26, 3, 0, // Skip to: 2190
/* 2187 */    MCD_OPC_Decode, 125, 1, // Opcode: LMBD_l1_rrr_x2
/* 2190 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 2198
/* 2194 */    MCD_OPC_Decode, 147, 2, 1, // Opcode: XOR_l1_rrr_x2
/* 2198 */    MCD_OPC_FilterValue, 30, 3, 0, // Skip to: 2205
/* 2202 */    MCD_OPC_Decode, 58, 1, // Opcode: AND_l1_rrr_x2
/* 2205 */    MCD_OPC_FilterValue, 31, 12, 0, // Skip to: 2221
/* 2209 */    MCD_OPC_Decode, 168, 1, 1, // Opcode: OR_l1_rrr_x2
/* 2213 */    MCD_OPC_FilterValue, 31, 4, 0, // Skip to: 2221
/* 2217 */    MCD_OPC_Decode, 240, 1, 5, // Opcode: STW_d6_rm
/* 2221 */    MCD_OPC_Fail,
  0
};

static bool checkDecoderPredicate(unsigned Idx, uint64_t Bits) {
  return true;
}

#define DecodeToMCInst(fname,fieldname, InsnType) \
static DecodeStatus fname(DecodeStatus S, unsigned Idx, InsnType insn, MCInst *MI, \
                                   uint64_t Address, void *Decoder) { \
  InsnType tmp; \
  switch (Idx) { \
  default: \
  case 0: \
    tmp = fieldname(insn, 13, 4); \
    if (DecodeNop(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 1: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 2: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeScst5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 3: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 7, 1) << 0; \
    tmp |= fieldname(insn, 9, 14) << 1; \
    if (DecodeMemOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 4: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 8, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 5: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 7, 16); \
    if (DecodeMemOperand2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 6: \
    tmp = fieldname(insn, 7, 21); \
    if (DecodePCRelScst21(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 7: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeScst5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 8: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 9: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 10: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeScst5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 11: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 12: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 13: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 10); \
    if (DecodePCRelScst10(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 14: \
    tmp = fieldname(insn, 16, 12); \
    if (DecodePCRelScst12(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 15: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeControlRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 16: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 17: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 18: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 19: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 20: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 21: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 22: \
    tmp = fieldname(insn, 24, 4); \
    if (DecodeRegPair4(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 7, 1) << 0; \
    tmp |= fieldname(insn, 9, 15) << 1; \
    if (DecodeMemOperandSc(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 23: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 7, 16); \
    if (DecodeScst16(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 24: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 25: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 26: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 27: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeScst5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 28: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 29: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 30: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 7, 1) << 0; \
    tmp |= fieldname(insn, 9, 14) << 1; \
    if (DecodeMemOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 31: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeScst5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 32: \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 33: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 7); \
    if (DecodePCRelScst7(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 34: \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 35: \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 3); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 36: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeControlRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 37: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 38: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeRegPair5(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX2(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 39: \
    tmp = fieldname(insn, 23, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 13, 5); \
    if (DecodeGPRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 1); \
    if (DecodeCrosspathX3(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 29, 3); \
    if (DecodeCondRegister(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 28, 1); \
    if (DecodeCondRegisterZero(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 1); \
    if (DecodeSide(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 1); \
    if (DecodeParallel(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  } \
}

#define DecodeInstruction(fname, fieldname, decoder, InsnType) \
static DecodeStatus fname(uint8_t DecodeTable[], MCInst *MI, \
                                      InsnType insn, uint64_t Address, \
                                      MCRegisterInfo *MRI, \
                                      int feature) { \
  uint64_t Bits = getFeatureBits(feature); \
  uint8_t *Ptr = DecodeTable; \
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
      CurFieldValue = (uint32_t) fieldname(insn, Start, Len); \
      break; \
    } \
    case MCD_OPC_FilterValue: { \
      Val = (InsnType) decodeULEB128(++Ptr, &Len); \
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
      ExpectedValue = (uint32_t) decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NumToSkip = *Ptr++; \
      NumToSkip |= (*Ptr++) << 8; \
      if (ExpectedValue != FieldValue) \
        Ptr += NumToSkip; \
      break; \
    } \
    case MCD_OPC_CheckPredicate: { \
      PIdx = (uint32_t) decodeULEB128(++Ptr, &Len); \
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
      Opc = (unsigned) decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      DecodeIdx = (unsigned) decodeULEB128(Ptr, &Len); \
      Ptr += Len; \
      MCInst_setOpcode(MI, Opc); \
      return decoder(S, DecodeIdx, insn, MI, Address, MRI); \
    } \
    case MCD_OPC_SoftFail: { \
      PositiveMask = (InsnType) decodeULEB128(++Ptr, &Len); \
      Ptr += Len; \
      NegativeMask = (InsnType) decodeULEB128(Ptr, &Len); \
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

FieldFromInstruction(fieldFromInstruction_4, uint32_t)
DecodeToMCInst(decodeToMCInst_4, fieldFromInstruction_4, uint32_t)
DecodeInstruction(decodeInstruction_4, fieldFromInstruction_4, decodeToMCInst_4, uint32_t)
