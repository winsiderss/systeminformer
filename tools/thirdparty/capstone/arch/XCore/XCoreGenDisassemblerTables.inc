/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* * XCore Disassembler                                                         *|
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
/* 0 */       MCD_OPC_ExtractField, 11, 5,  // Inst{15-11} ...
/* 3 */       MCD_OPC_FilterValue, 0, 108, 0, // Skip to: 115
/* 7 */       MCD_OPC_ExtractField, 0, 11,  // Inst{10-0} ...
/* 10 */      MCD_OPC_FilterValue, 236, 15, 4, 0, // Skip to: 19
/* 15 */      MCD_OPC_Decode, 243, 1, 0, // Opcode: WAITEU_0R
/* 19 */      MCD_OPC_FilterValue, 237, 15, 3, 0, // Skip to: 27
/* 24 */      MCD_OPC_Decode, 59, 0, // Opcode: CLRE_0R
/* 27 */      MCD_OPC_FilterValue, 238, 15, 4, 0, // Skip to: 36
/* 32 */      MCD_OPC_Decode, 218, 1, 0, // Opcode: SSYNC_0r
/* 36 */      MCD_OPC_FilterValue, 239, 15, 3, 0, // Skip to: 44
/* 41 */      MCD_OPC_Decode, 93, 0, // Opcode: FREET_0R
/* 44 */      MCD_OPC_FilterValue, 252, 15, 3, 0, // Skip to: 52
/* 49 */      MCD_OPC_Decode, 68, 0, // Opcode: DCALL_0R
/* 52 */      MCD_OPC_FilterValue, 253, 15, 3, 0, // Skip to: 60
/* 57 */      MCD_OPC_Decode, 125, 0, // Opcode: KRET_0R
/* 60 */      MCD_OPC_FilterValue, 254, 15, 3, 0, // Skip to: 68
/* 65 */      MCD_OPC_Decode, 74, 0, // Opcode: DRET_0R
/* 68 */      MCD_OPC_FilterValue, 255, 15, 4, 0, // Skip to: 77
/* 73 */      MCD_OPC_Decode, 199, 1, 0, // Opcode: SETKEP_0R
/* 77 */      MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 80 */      MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 87
/* 84 */      MCD_OPC_Decode, 77, 1, // Opcode: EDU_1r
/* 87 */      MCD_OPC_FilterValue, 127, 3, 0, // Skip to: 94
/* 91 */      MCD_OPC_Decode, 80, 1, // Opcode: EEU_1r
/* 94 */      MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 97 */      MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 104
/* 101 */     MCD_OPC_Decode, 111, 2, // Opcode: INITPC_2r
/* 104 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 111
/* 108 */     MCD_OPC_Decode, 105, 2, // Opcode: GETST_2r
/* 111 */     MCD_OPC_Decode, 230, 1, 3, // Opcode: STW_2rus
/* 115 */     MCD_OPC_FilterValue, 1, 114, 0, // Skip to: 233
/* 119 */     MCD_OPC_ExtractField, 0, 11,  // Inst{10-0} ...
/* 122 */     MCD_OPC_FilterValue, 236, 15, 4, 0, // Skip to: 131
/* 127 */     MCD_OPC_Decode, 152, 1, 0, // Opcode: LDSPC_0R
/* 131 */     MCD_OPC_FilterValue, 237, 15, 4, 0, // Skip to: 140
/* 136 */     MCD_OPC_Decode, 223, 1, 0, // Opcode: STSPC_0R
/* 140 */     MCD_OPC_FilterValue, 238, 15, 4, 0, // Skip to: 149
/* 145 */     MCD_OPC_Decode, 153, 1, 0, // Opcode: LDSSR_0R
/* 149 */     MCD_OPC_FilterValue, 239, 15, 4, 0, // Skip to: 158
/* 154 */     MCD_OPC_Decode, 224, 1, 0, // Opcode: STSSR_0R
/* 158 */     MCD_OPC_FilterValue, 252, 15, 4, 0, // Skip to: 167
/* 163 */     MCD_OPC_Decode, 222, 1, 0, // Opcode: STSED_0R
/* 167 */     MCD_OPC_FilterValue, 253, 15, 4, 0, // Skip to: 176
/* 172 */     MCD_OPC_Decode, 221, 1, 0, // Opcode: STET_0R
/* 176 */     MCD_OPC_FilterValue, 254, 15, 3, 0, // Skip to: 184
/* 181 */     MCD_OPC_Decode, 95, 0, // Opcode: GETED_0R
/* 184 */     MCD_OPC_FilterValue, 255, 15, 3, 0, // Skip to: 192
/* 189 */     MCD_OPC_Decode, 96, 0, // Opcode: GETET_0R
/* 192 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 195 */     MCD_OPC_FilterValue, 126, 4, 0, // Skip to: 203
/* 199 */     MCD_OPC_Decode, 242, 1, 1, // Opcode: WAITET_1R
/* 203 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 211
/* 207 */     MCD_OPC_Decode, 241, 1, 1, // Opcode: WAITEF_1R
/* 211 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 214 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 221
/* 218 */     MCD_OPC_Decode, 109, 2, // Opcode: INITDP_2r
/* 221 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 229
/* 225 */     MCD_OPC_Decode, 183, 1, 4, // Opcode: OUTT_2r
/* 229 */     MCD_OPC_Decode, 163, 1, 3, // Opcode: LDW_2rus
/* 233 */     MCD_OPC_FilterValue, 2, 100, 0, // Skip to: 337
/* 237 */     MCD_OPC_ExtractField, 0, 11,  // Inst{10-0} ...
/* 240 */     MCD_OPC_FilterValue, 236, 15, 3, 0, // Skip to: 248
/* 245 */     MCD_OPC_Decode, 69, 0, // Opcode: DENTSP_0R
/* 248 */     MCD_OPC_FilterValue, 237, 15, 3, 0, // Skip to: 256
/* 253 */     MCD_OPC_Decode, 73, 0, // Opcode: DRESTSP_0R
/* 256 */     MCD_OPC_FilterValue, 238, 15, 3, 0, // Skip to: 264
/* 261 */     MCD_OPC_Decode, 97, 0, // Opcode: GETID_0R
/* 264 */     MCD_OPC_FilterValue, 239, 15, 3, 0, // Skip to: 272
/* 269 */     MCD_OPC_Decode, 98, 0, // Opcode: GETKEP_0R
/* 272 */     MCD_OPC_FilterValue, 252, 15, 3, 0, // Skip to: 280
/* 277 */     MCD_OPC_Decode, 99, 0, // Opcode: GETKSP_0R
/* 280 */     MCD_OPC_FilterValue, 253, 15, 4, 0, // Skip to: 289
/* 285 */     MCD_OPC_Decode, 151, 1, 0, // Opcode: LDSED_0R
/* 289 */     MCD_OPC_FilterValue, 254, 15, 4, 0, // Skip to: 298
/* 294 */     MCD_OPC_Decode, 149, 1, 0, // Opcode: LDET_0R
/* 298 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 301 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 308
/* 305 */     MCD_OPC_Decode, 92, 1, // Opcode: FREER_1r
/* 308 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 316
/* 312 */     MCD_OPC_Decode, 171, 1, 1, // Opcode: MJOIN_1r
/* 316 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 319 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 326
/* 323 */     MCD_OPC_Decode, 112, 2, // Opcode: INITSP_2r
/* 326 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 334
/* 330 */     MCD_OPC_Decode, 197, 1, 4, // Opcode: SETD_2r
/* 334 */     MCD_OPC_Decode, 23, 5, // Opcode: ADD_3r
/* 337 */     MCD_OPC_FilterValue, 3, 41, 0, // Skip to: 382
/* 341 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 344 */     MCD_OPC_FilterValue, 126, 4, 0, // Skip to: 352
/* 348 */     MCD_OPC_Decode, 240, 1, 1, // Opcode: TSTART_1R
/* 352 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 360
/* 356 */     MCD_OPC_Decode, 174, 1, 1, // Opcode: MSYNC_1r
/* 360 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 363 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 370
/* 367 */     MCD_OPC_Decode, 108, 2, // Opcode: INITCP_2r
/* 370 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 378
/* 374 */     MCD_OPC_Decode, 238, 1, 6, // Opcode: TSETMR_2r
/* 378 */     MCD_OPC_Decode, 233, 1, 5, // Opcode: SUB_3r
/* 382 */     MCD_OPC_FilterValue, 4, 30, 0, // Skip to: 416
/* 386 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 389 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 396
/* 393 */     MCD_OPC_Decode, 36, 1, // Opcode: BLA_1r
/* 396 */     MCD_OPC_FilterValue, 127, 3, 0, // Skip to: 403
/* 400 */     MCD_OPC_Decode, 30, 1, // Opcode: BAU_1r
/* 403 */     MCD_OPC_CheckField, 4, 1, 1, 3, 0, // Skip to: 412
/* 409 */     MCD_OPC_Decode, 79, 2, // Opcode: EET_2r
/* 412 */     MCD_OPC_Decode, 215, 1, 5, // Opcode: SHL_3r
/* 416 */     MCD_OPC_FilterValue, 5, 39, 0, // Skip to: 459
/* 420 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 423 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 430
/* 427 */     MCD_OPC_Decode, 53, 1, // Opcode: BRU_1r
/* 430 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 438
/* 434 */     MCD_OPC_Decode, 205, 1, 1, // Opcode: SETSP_1r
/* 438 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 441 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 448
/* 445 */     MCD_OPC_Decode, 26, 7, // Opcode: ANDNOT_2r
/* 448 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 455
/* 452 */     MCD_OPC_Decode, 78, 2, // Opcode: EEF_2r
/* 455 */     MCD_OPC_Decode, 217, 1, 5, // Opcode: SHR_3r
/* 459 */     MCD_OPC_FilterValue, 6, 41, 0, // Skip to: 504
/* 463 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 466 */     MCD_OPC_FilterValue, 126, 4, 0, // Skip to: 474
/* 470 */     MCD_OPC_Decode, 196, 1, 1, // Opcode: SETDP_1r
/* 474 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 482
/* 478 */     MCD_OPC_Decode, 192, 1, 1, // Opcode: SETCP_1r
/* 482 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 485 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 493
/* 489 */     MCD_OPC_Decode, 212, 1, 7, // Opcode: SEXT_2r
/* 493 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 501
/* 497 */     MCD_OPC_Decode, 213, 1, 8, // Opcode: SEXT_rus
/* 501 */     MCD_OPC_Decode, 86, 5, // Opcode: EQ_3r
/* 504 */     MCD_OPC_FilterValue, 7, 39, 0, // Skip to: 547
/* 508 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 511 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 518
/* 515 */     MCD_OPC_Decode, 70, 1, // Opcode: DGETREG_1r
/* 518 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 526
/* 522 */     MCD_OPC_Decode, 198, 1, 1, // Opcode: SETEV_1r
/* 526 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 529 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 536
/* 533 */     MCD_OPC_Decode, 106, 2, // Opcode: GETTS_2r
/* 536 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 544
/* 540 */     MCD_OPC_Decode, 203, 1, 4, // Opcode: SETPT_2r
/* 544 */     MCD_OPC_Decode, 27, 5, // Opcode: AND_3r
/* 547 */     MCD_OPC_FilterValue, 8, 41, 0, // Skip to: 592
/* 551 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 554 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 561
/* 558 */     MCD_OPC_Decode, 118, 1, // Opcode: KCALL_1r
/* 561 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 569
/* 565 */     MCD_OPC_Decode, 211, 1, 1, // Opcode: SETV_1r
/* 569 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 572 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 580
/* 576 */     MCD_OPC_Decode, 245, 1, 7, // Opcode: ZEXT_2r
/* 580 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 588
/* 584 */     MCD_OPC_Decode, 246, 1, 8, // Opcode: ZEXT_rus
/* 588 */     MCD_OPC_Decode, 178, 1, 5, // Opcode: OR_3r
/* 592 */     MCD_OPC_FilterValue, 9, 40, 0, // Skip to: 636
/* 596 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 599 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 606
/* 603 */     MCD_OPC_Decode, 75, 1, // Opcode: ECALLF_1r
/* 606 */     MCD_OPC_FilterValue, 127, 3, 0, // Skip to: 613
/* 610 */     MCD_OPC_Decode, 76, 1, // Opcode: ECALLT_1r
/* 613 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 616 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 624
/* 620 */     MCD_OPC_Decode, 179, 1, 2, // Opcode: OUTCT_2r
/* 624 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 632
/* 628 */     MCD_OPC_Decode, 180, 1, 9, // Opcode: OUTCT_rus
/* 632 */     MCD_OPC_Decode, 164, 1, 5, // Opcode: LDW_3r
/* 636 */     MCD_OPC_FilterValue, 10, 19, 0, // Skip to: 659
/* 640 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 643 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 651
/* 647 */     MCD_OPC_Decode, 226, 1, 10, // Opcode: STWDP_ru6
/* 651 */     MCD_OPC_FilterValue, 1, 54, 2, // Skip to: 1221
/* 655 */     MCD_OPC_Decode, 229, 1, 10, // Opcode: STWSP_ru6
/* 659 */     MCD_OPC_FilterValue, 11, 19, 0, // Skip to: 682
/* 663 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 666 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 674
/* 670 */     MCD_OPC_Decode, 159, 1, 10, // Opcode: LDWDP_ru6
/* 674 */     MCD_OPC_FilterValue, 1, 31, 2, // Skip to: 1221
/* 678 */     MCD_OPC_Decode, 162, 1, 10, // Opcode: LDWSP_ru6
/* 682 */     MCD_OPC_FilterValue, 12, 19, 0, // Skip to: 705
/* 686 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 689 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 697
/* 693 */     MCD_OPC_Decode, 141, 1, 10, // Opcode: LDAWDP_ru6
/* 697 */     MCD_OPC_FilterValue, 1, 8, 2, // Skip to: 1221
/* 701 */     MCD_OPC_Decode, 146, 1, 10, // Opcode: LDAWSP_ru6
/* 705 */     MCD_OPC_FilterValue, 13, 19, 0, // Skip to: 728
/* 709 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 712 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 720
/* 716 */     MCD_OPC_Decode, 148, 1, 10, // Opcode: LDC_ru6
/* 720 */     MCD_OPC_FilterValue, 1, 241, 1, // Skip to: 1221
/* 724 */     MCD_OPC_Decode, 156, 1, 10, // Opcode: LDWCP_ru6
/* 728 */     MCD_OPC_FilterValue, 14, 80, 0, // Skip to: 812
/* 732 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 735 */     MCD_OPC_FilterValue, 0, 34, 0, // Skip to: 773
/* 739 */     MCD_OPC_ExtractField, 6, 4,  // Inst{9-6} ...
/* 742 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 749
/* 746 */     MCD_OPC_Decode, 52, 11, // Opcode: BRFU_u6
/* 749 */     MCD_OPC_FilterValue, 13, 3, 0, // Skip to: 756
/* 753 */     MCD_OPC_Decode, 35, 11, // Opcode: BLAT_u6
/* 756 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 763
/* 760 */     MCD_OPC_Decode, 88, 11, // Opcode: EXTDP_u6
/* 763 */     MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 770
/* 767 */     MCD_OPC_Decode, 120, 11, // Opcode: KCALL_u6
/* 770 */     MCD_OPC_Decode, 50, 12, // Opcode: BRFT_ru6
/* 773 */     MCD_OPC_FilterValue, 1, 188, 1, // Skip to: 1221
/* 777 */     MCD_OPC_ExtractField, 6, 4,  // Inst{9-6} ...
/* 780 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 787
/* 784 */     MCD_OPC_Decode, 46, 13, // Opcode: BRBU_u6
/* 787 */     MCD_OPC_FilterValue, 13, 3, 0, // Skip to: 794
/* 791 */     MCD_OPC_Decode, 84, 11, // Opcode: ENTSP_u6
/* 794 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 801
/* 798 */     MCD_OPC_Decode, 90, 11, // Opcode: EXTSP_u6
/* 801 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 809
/* 805 */     MCD_OPC_Decode, 189, 1, 11, // Opcode: RETSP_u6
/* 809 */     MCD_OPC_Decode, 44, 14, // Opcode: BRBT_ru6
/* 812 */     MCD_OPC_FilterValue, 15, 67, 0, // Skip to: 883
/* 816 */     MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 819 */     MCD_OPC_FilterValue, 0, 35, 0, // Skip to: 858
/* 823 */     MCD_OPC_ExtractField, 6, 4,  // Inst{9-6} ...
/* 826 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 833
/* 830 */     MCD_OPC_Decode, 64, 11, // Opcode: CLRSR_u6
/* 833 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 841
/* 837 */     MCD_OPC_Decode, 209, 1, 11, // Opcode: SETSR_u6
/* 841 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 848
/* 845 */     MCD_OPC_Decode, 122, 11, // Opcode: KENTSP_u6
/* 848 */     MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 855
/* 852 */     MCD_OPC_Decode, 124, 11, // Opcode: KRESTSP_u6
/* 855 */     MCD_OPC_Decode, 48, 12, // Opcode: BRFF_ru6
/* 858 */     MCD_OPC_FilterValue, 1, 103, 1, // Skip to: 1221
/* 862 */     MCD_OPC_ExtractField, 6, 4,  // Inst{9-6} ...
/* 865 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 872
/* 869 */     MCD_OPC_Decode, 104, 11, // Opcode: GETSR_u6
/* 872 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 880
/* 876 */     MCD_OPC_Decode, 139, 1, 11, // Opcode: LDAWCP_u6
/* 880 */     MCD_OPC_Decode, 42, 14, // Opcode: BRBF_ru6
/* 883 */     MCD_OPC_FilterValue, 16, 38, 0, // Skip to: 925
/* 887 */     MCD_OPC_ExtractField, 4, 7,  // Inst{10-4} ...
/* 890 */     MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 897
/* 894 */     MCD_OPC_Decode, 60, 1, // Opcode: CLRPT_1R
/* 897 */     MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 905
/* 901 */     MCD_OPC_Decode, 234, 1, 1, // Opcode: SYNCR_1r
/* 905 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 908 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 915
/* 912 */     MCD_OPC_Decode, 102, 9, // Opcode: GETR_rus
/* 915 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 922
/* 919 */     MCD_OPC_Decode, 107, 2, // Opcode: INCT_2r
/* 922 */     MCD_OPC_Decode, 127, 5, // Opcode: LD16S_3r
/* 925 */     MCD_OPC_FilterValue, 17, 22, 0, // Skip to: 951
/* 929 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 932 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 940
/* 936 */     MCD_OPC_Decode, 177, 1, 2, // Opcode: NOT
/* 940 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 947
/* 944 */     MCD_OPC_Decode, 115, 2, // Opcode: INT_2r
/* 947 */     MCD_OPC_Decode, 128, 1, 5, // Opcode: LD8U_3r
/* 951 */     MCD_OPC_FilterValue, 18, 21, 0, // Skip to: 976
/* 955 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 958 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 966
/* 962 */     MCD_OPC_Decode, 176, 1, 2, // Opcode: NEG
/* 966 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 973
/* 970 */     MCD_OPC_Decode, 82, 2, // Opcode: ENDIN_2r
/* 973 */     MCD_OPC_Decode, 22, 3, // Opcode: ADD_2rus
/* 976 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 984
/* 980 */     MCD_OPC_Decode, 232, 1, 3, // Opcode: SUB_2rus
/* 984 */     MCD_OPC_FilterValue, 20, 23, 0, // Skip to: 1011
/* 988 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 991 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 999
/* 995 */     MCD_OPC_Decode, 172, 1, 2, // Opcode: MKMSK_2r
/* 999 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1007
/* 1003 */    MCD_OPC_Decode, 173, 1, 15, // Opcode: MKMSK_rus
/* 1007 */    MCD_OPC_Decode, 214, 1, 16, // Opcode: SHL_2rus
/* 1011 */    MCD_OPC_FilterValue, 21, 23, 0, // Skip to: 1038
/* 1015 */    MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 1018 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1026
/* 1022 */    MCD_OPC_Decode, 184, 1, 4, // Opcode: OUT_2r
/* 1026 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1034
/* 1030 */    MCD_OPC_Decode, 182, 1, 7, // Opcode: OUTSHR_2r
/* 1034 */    MCD_OPC_Decode, 216, 1, 16, // Opcode: SHR_2rus
/* 1038 */    MCD_OPC_FilterValue, 22, 20, 0, // Skip to: 1062
/* 1042 */    MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 1045 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1052
/* 1049 */    MCD_OPC_Decode, 116, 2, // Opcode: IN_2r
/* 1052 */    MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 1059
/* 1056 */    MCD_OPC_Decode, 114, 7, // Opcode: INSHR_2r
/* 1059 */    MCD_OPC_Decode, 85, 3, // Opcode: EQ_2rus
/* 1062 */    MCD_OPC_FilterValue, 23, 23, 0, // Skip to: 1089
/* 1066 */    MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 1069 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1077
/* 1073 */    MCD_OPC_Decode, 185, 1, 2, // Opcode: PEEK_2r
/* 1077 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1085
/* 1081 */    MCD_OPC_Decode, 235, 1, 2, // Opcode: TESTCT_2r
/* 1085 */    MCD_OPC_Decode, 239, 1, 17, // Opcode: TSETR_3r
/* 1089 */    MCD_OPC_FilterValue, 24, 23, 0, // Skip to: 1116
/* 1093 */    MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 1096 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1104
/* 1100 */    MCD_OPC_Decode, 201, 1, 4, // Opcode: SETPSC_2r
/* 1104 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1112
/* 1108 */    MCD_OPC_Decode, 237, 1, 2, // Opcode: TESTWCT_2r
/* 1112 */    MCD_OPC_Decode, 166, 1, 5, // Opcode: LSS_3r
/* 1116 */    MCD_OPC_FilterValue, 25, 21, 0, // Skip to: 1141
/* 1120 */    MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 1123 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1130
/* 1127 */    MCD_OPC_Decode, 57, 2, // Opcode: CHKCT_2r
/* 1130 */    MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 1137
/* 1134 */    MCD_OPC_Decode, 58, 15, // Opcode: CHKCT_rus
/* 1137 */    MCD_OPC_Decode, 168, 1, 5, // Opcode: LSU_3r
/* 1141 */    MCD_OPC_FilterValue, 26, 17, 0, // Skip to: 1162
/* 1145 */    MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 1148 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1155
/* 1152 */    MCD_OPC_Decode, 40, 18, // Opcode: BLRF_u10
/* 1155 */    MCD_OPC_FilterValue, 1, 62, 0, // Skip to: 1221
/* 1159 */    MCD_OPC_Decode, 38, 19, // Opcode: BLRB_u10
/* 1162 */    MCD_OPC_FilterValue, 27, 19, 0, // Skip to: 1185
/* 1166 */    MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 1169 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1177
/* 1173 */    MCD_OPC_Decode, 135, 1, 18, // Opcode: LDAPF_u10
/* 1177 */    MCD_OPC_FilterValue, 1, 40, 0, // Skip to: 1221
/* 1181 */    MCD_OPC_Decode, 132, 1, 19, // Opcode: LDAPB_u10
/* 1185 */    MCD_OPC_FilterValue, 28, 18, 0, // Skip to: 1207
/* 1189 */    MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 1192 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 1199
/* 1196 */    MCD_OPC_Decode, 33, 18, // Opcode: BLACP_u10
/* 1199 */    MCD_OPC_FilterValue, 1, 18, 0, // Skip to: 1221
/* 1203 */    MCD_OPC_Decode, 157, 1, 18, // Opcode: LDWCP_u10
/* 1207 */    MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 1221
/* 1211 */    MCD_OPC_CheckField, 10, 1, 0, 4, 0, // Skip to: 1221
/* 1217 */    MCD_OPC_Decode, 195, 1, 12, // Opcode: SETC_ru6
/* 1221 */    MCD_OPC_Fail,
  0
};

static const uint8_t DecoderTable32[] = {
/* 0 */       MCD_OPC_ExtractField, 27, 5,  // Inst{31-27} ...
/* 3 */       MCD_OPC_FilterValue, 0, 89, 0, // Skip to: 96
/* 7 */       MCD_OPC_ExtractField, 11, 5,  // Inst{15-11} ...
/* 10 */      MCD_OPC_FilterValue, 31, 216, 3, // Skip to: 998
/* 14 */      MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 17 */      MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 31
/* 21 */      MCD_OPC_CheckField, 16, 11, 236, 15, 17, 0, // Skip to: 45
/* 28 */      MCD_OPC_Decode, 31, 20, // Opcode: BITREV_l2r
/* 31 */      MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 45
/* 35 */      MCD_OPC_CheckField, 16, 11, 236, 15, 3, 0, // Skip to: 45
/* 42 */      MCD_OPC_Decode, 56, 20, // Opcode: BYTEREV_l2r
/* 45 */      MCD_OPC_CheckField, 16, 11, 236, 15, 4, 0, // Skip to: 56
/* 52 */      MCD_OPC_Decode, 231, 1, 21, // Opcode: STW_l3r
/* 56 */      MCD_OPC_ExtractField, 20, 7,  // Inst{26-20} ...
/* 59 */      MCD_OPC_FilterValue, 126, 3, 0, // Skip to: 66
/* 63 */      MCD_OPC_Decode, 66, 22, // Opcode: CRC8_l4r
/* 66 */      MCD_OPC_FilterValue, 127, 4, 0, // Skip to: 74
/* 70 */      MCD_OPC_Decode, 170, 1, 23, // Opcode: MACCU_l4r
/* 74 */      MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 77 */      MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 85
/* 81 */      MCD_OPC_Decode, 150, 1, 24, // Opcode: LDIVU_l5r
/* 85 */      MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 92
/* 89 */      MCD_OPC_Decode, 126, 24, // Opcode: LADD_l5r
/* 92 */      MCD_OPC_Decode, 165, 1, 25, // Opcode: LMUL_l6r
/* 96 */      MCD_OPC_FilterValue, 1, 86, 0, // Skip to: 186
/* 100 */     MCD_OPC_ExtractField, 11, 5,  // Inst{15-11} ...
/* 103 */     MCD_OPC_FilterValue, 31, 123, 3, // Skip to: 998
/* 107 */     MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 110 */     MCD_OPC_FilterValue, 0, 116, 3, // Skip to: 998
/* 114 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 117 */     MCD_OPC_FilterValue, 0, 15, 0, // Skip to: 136
/* 121 */     MCD_OPC_CheckField, 21, 6, 63, 29, 0, // Skip to: 156
/* 127 */     MCD_OPC_CheckField, 16, 4, 12, 23, 0, // Skip to: 156
/* 133 */     MCD_OPC_Decode, 65, 20, // Opcode: CLZ_l2r
/* 136 */     MCD_OPC_FilterValue, 1, 16, 0, // Skip to: 156
/* 140 */     MCD_OPC_CheckField, 21, 6, 63, 10, 0, // Skip to: 156
/* 146 */     MCD_OPC_CheckField, 16, 4, 12, 4, 0, // Skip to: 156
/* 152 */     MCD_OPC_Decode, 191, 1, 26, // Opcode: SETCLK_l2r
/* 156 */     MCD_OPC_CheckField, 21, 6, 63, 10, 0, // Skip to: 172
/* 162 */     MCD_OPC_CheckField, 16, 4, 12, 4, 0, // Skip to: 172
/* 168 */     MCD_OPC_Decode, 244, 1, 21, // Opcode: XOR_l3r
/* 172 */     MCD_OPC_CheckField, 21, 6, 63, 4, 0, // Skip to: 182
/* 178 */     MCD_OPC_Decode, 169, 1, 23, // Opcode: MACCS_l4r
/* 182 */     MCD_OPC_Decode, 167, 1, 24, // Opcode: LSUB_l5r
/* 186 */     MCD_OPC_FilterValue, 2, 29, 0, // Skip to: 219
/* 190 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 193 */     MCD_OPC_FilterValue, 159, 251, 3, 31, 3, // Skip to: 998
/* 199 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 202 */     MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 209
/* 206 */     MCD_OPC_Decode, 110, 20, // Opcode: INITLR_l2r
/* 209 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 216
/* 213 */     MCD_OPC_Decode, 101, 20, // Opcode: GETPS_l2r
/* 216 */     MCD_OPC_Decode, 29, 21, // Opcode: ASHR_l3r
/* 219 */     MCD_OPC_FilterValue, 3, 31, 0, // Skip to: 254
/* 223 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 226 */     MCD_OPC_FilterValue, 159, 251, 3, 254, 2, // Skip to: 998
/* 232 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 235 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 243
/* 239 */     MCD_OPC_Decode, 202, 1, 26, // Opcode: SETPS_l2r
/* 243 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 250
/* 247 */     MCD_OPC_Decode, 94, 20, // Opcode: GETD_l2r
/* 250 */     MCD_OPC_Decode, 144, 1, 21, // Opcode: LDAWF_l3r
/* 254 */     MCD_OPC_FilterValue, 4, 32, 0, // Skip to: 290
/* 258 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 261 */     MCD_OPC_FilterValue, 159, 251, 3, 219, 2, // Skip to: 998
/* 267 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 270 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 278
/* 274 */     MCD_OPC_Decode, 236, 1, 20, // Opcode: TESTLCL_l2r
/* 278 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 286
/* 282 */     MCD_OPC_Decode, 210, 1, 26, // Opcode: SETTW_l2r
/* 286 */     MCD_OPC_Decode, 137, 1, 21, // Opcode: LDAWB_l3r
/* 290 */     MCD_OPC_FilterValue, 5, 32, 0, // Skip to: 326
/* 294 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 297 */     MCD_OPC_FilterValue, 159, 251, 3, 183, 2, // Skip to: 998
/* 303 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 306 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 314
/* 310 */     MCD_OPC_Decode, 204, 1, 26, // Opcode: SETRDY_l2r
/* 314 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 322
/* 318 */     MCD_OPC_Decode, 193, 1, 20, // Opcode: SETC_l2r
/* 322 */     MCD_OPC_Decode, 130, 1, 21, // Opcode: LDA16F_l3r
/* 326 */     MCD_OPC_FilterValue, 6, 31, 0, // Skip to: 361
/* 330 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 333 */     MCD_OPC_FilterValue, 159, 251, 3, 147, 2, // Skip to: 998
/* 339 */     MCD_OPC_ExtractField, 4, 1,  // Inst{4} ...
/* 342 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 350
/* 346 */     MCD_OPC_Decode, 200, 1, 26, // Opcode: SETN_l2r
/* 350 */     MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 357
/* 354 */     MCD_OPC_Decode, 100, 20, // Opcode: GETN_l2r
/* 357 */     MCD_OPC_Decode, 129, 1, 21, // Opcode: LDA16B_l3r
/* 361 */     MCD_OPC_FilterValue, 7, 12, 0, // Skip to: 377
/* 365 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 113, 2, // Skip to: 998
/* 373 */     MCD_OPC_Decode, 175, 1, 21, // Opcode: MUL_l3r
/* 377 */     MCD_OPC_FilterValue, 8, 11, 0, // Skip to: 392
/* 381 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 97, 2, // Skip to: 998
/* 389 */     MCD_OPC_Decode, 71, 21, // Opcode: DIVS_l3r
/* 392 */     MCD_OPC_FilterValue, 9, 11, 0, // Skip to: 407
/* 396 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 82, 2, // Skip to: 998
/* 404 */     MCD_OPC_Decode, 72, 21, // Opcode: DIVU_l3r
/* 407 */     MCD_OPC_FilterValue, 10, 31, 0, // Skip to: 442
/* 411 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 414 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 428
/* 418 */     MCD_OPC_CheckField, 10, 6, 60, 62, 2, // Skip to: 998
/* 424 */     MCD_OPC_Decode, 225, 1, 27, // Opcode: STWDP_lru6
/* 428 */     MCD_OPC_FilterValue, 1, 54, 2, // Skip to: 998
/* 432 */     MCD_OPC_CheckField, 10, 6, 60, 48, 2, // Skip to: 998
/* 438 */     MCD_OPC_Decode, 228, 1, 27, // Opcode: STWSP_lru6
/* 442 */     MCD_OPC_FilterValue, 11, 31, 0, // Skip to: 477
/* 446 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 449 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 463
/* 453 */     MCD_OPC_CheckField, 10, 6, 60, 27, 2, // Skip to: 998
/* 459 */     MCD_OPC_Decode, 158, 1, 27, // Opcode: LDWDP_lru6
/* 463 */     MCD_OPC_FilterValue, 1, 19, 2, // Skip to: 998
/* 467 */     MCD_OPC_CheckField, 10, 6, 60, 13, 2, // Skip to: 998
/* 473 */     MCD_OPC_Decode, 161, 1, 27, // Opcode: LDWSP_lru6
/* 477 */     MCD_OPC_FilterValue, 12, 31, 0, // Skip to: 512
/* 481 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 484 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 498
/* 488 */     MCD_OPC_CheckField, 10, 6, 60, 248, 1, // Skip to: 998
/* 494 */     MCD_OPC_Decode, 140, 1, 27, // Opcode: LDAWDP_lru6
/* 498 */     MCD_OPC_FilterValue, 1, 240, 1, // Skip to: 998
/* 502 */     MCD_OPC_CheckField, 10, 6, 60, 234, 1, // Skip to: 998
/* 508 */     MCD_OPC_Decode, 145, 1, 27, // Opcode: LDAWSP_lru6
/* 512 */     MCD_OPC_FilterValue, 13, 31, 0, // Skip to: 547
/* 516 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 519 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 533
/* 523 */     MCD_OPC_CheckField, 10, 6, 60, 213, 1, // Skip to: 998
/* 529 */     MCD_OPC_Decode, 147, 1, 27, // Opcode: LDC_lru6
/* 533 */     MCD_OPC_FilterValue, 1, 205, 1, // Skip to: 998
/* 537 */     MCD_OPC_CheckField, 10, 6, 60, 199, 1, // Skip to: 998
/* 543 */     MCD_OPC_Decode, 154, 1, 27, // Opcode: LDWCP_lru6
/* 547 */     MCD_OPC_FilterValue, 14, 94, 0, // Skip to: 645
/* 551 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 554 */     MCD_OPC_FilterValue, 0, 41, 0, // Skip to: 599
/* 558 */     MCD_OPC_ExtractField, 10, 6,  // Inst{15-10} ...
/* 561 */     MCD_OPC_FilterValue, 60, 177, 1, // Skip to: 998
/* 565 */     MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 568 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 575
/* 572 */     MCD_OPC_Decode, 51, 28, // Opcode: BRFU_lu6
/* 575 */     MCD_OPC_FilterValue, 13, 3, 0, // Skip to: 582
/* 579 */     MCD_OPC_Decode, 34, 28, // Opcode: BLAT_lu6
/* 582 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 589
/* 586 */     MCD_OPC_Decode, 87, 28, // Opcode: EXTDP_lu6
/* 589 */     MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 596
/* 593 */     MCD_OPC_Decode, 119, 28, // Opcode: KCALL_lu6
/* 596 */     MCD_OPC_Decode, 49, 29, // Opcode: BRFT_lru6
/* 599 */     MCD_OPC_FilterValue, 1, 139, 1, // Skip to: 998
/* 603 */     MCD_OPC_ExtractField, 10, 6,  // Inst{15-10} ...
/* 606 */     MCD_OPC_FilterValue, 60, 132, 1, // Skip to: 998
/* 610 */     MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 613 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 620
/* 617 */     MCD_OPC_Decode, 45, 30, // Opcode: BRBU_lu6
/* 620 */     MCD_OPC_FilterValue, 13, 3, 0, // Skip to: 627
/* 624 */     MCD_OPC_Decode, 83, 28, // Opcode: ENTSP_lu6
/* 627 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 634
/* 631 */     MCD_OPC_Decode, 89, 28, // Opcode: EXTSP_lu6
/* 634 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 642
/* 638 */     MCD_OPC_Decode, 188, 1, 28, // Opcode: RETSP_lu6
/* 642 */     MCD_OPC_Decode, 43, 31, // Opcode: BRBT_lru6
/* 645 */     MCD_OPC_FilterValue, 15, 81, 0, // Skip to: 730
/* 649 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 652 */     MCD_OPC_FilterValue, 0, 42, 0, // Skip to: 698
/* 656 */     MCD_OPC_ExtractField, 10, 6,  // Inst{15-10} ...
/* 659 */     MCD_OPC_FilterValue, 60, 79, 1, // Skip to: 998
/* 663 */     MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 666 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 673
/* 670 */     MCD_OPC_Decode, 63, 28, // Opcode: CLRSR_lu6
/* 673 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 681
/* 677 */     MCD_OPC_Decode, 208, 1, 28, // Opcode: SETSR_lu6
/* 681 */     MCD_OPC_FilterValue, 14, 3, 0, // Skip to: 688
/* 685 */     MCD_OPC_Decode, 121, 28, // Opcode: KENTSP_lu6
/* 688 */     MCD_OPC_FilterValue, 15, 3, 0, // Skip to: 695
/* 692 */     MCD_OPC_Decode, 123, 28, // Opcode: KRESTSP_lu6
/* 695 */     MCD_OPC_Decode, 47, 29, // Opcode: BRFF_lru6
/* 698 */     MCD_OPC_FilterValue, 1, 40, 1, // Skip to: 998
/* 702 */     MCD_OPC_ExtractField, 10, 6,  // Inst{15-10} ...
/* 705 */     MCD_OPC_FilterValue, 60, 33, 1, // Skip to: 998
/* 709 */     MCD_OPC_ExtractField, 22, 4,  // Inst{25-22} ...
/* 712 */     MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 719
/* 716 */     MCD_OPC_Decode, 103, 28, // Opcode: GETSR_lu6
/* 719 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 727
/* 723 */     MCD_OPC_Decode, 138, 1, 28, // Opcode: LDAWCP_lu6
/* 727 */     MCD_OPC_Decode, 41, 31, // Opcode: BRBF_lru6
/* 730 */     MCD_OPC_FilterValue, 16, 12, 0, // Skip to: 746
/* 734 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 0, 1, // Skip to: 998
/* 742 */     MCD_OPC_Decode, 219, 1, 21, // Opcode: ST16_l3r
/* 746 */     MCD_OPC_FilterValue, 17, 12, 0, // Skip to: 762
/* 750 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 240, 0, // Skip to: 998
/* 758 */     MCD_OPC_Decode, 220, 1, 21, // Opcode: ST8_l3r
/* 762 */     MCD_OPC_FilterValue, 18, 31, 0, // Skip to: 797
/* 766 */     MCD_OPC_ExtractField, 11, 16,  // Inst{26-11} ...
/* 769 */     MCD_OPC_FilterValue, 159, 251, 3, 3, 0, // Skip to: 778
/* 775 */     MCD_OPC_Decode, 28, 32, // Opcode: ASHR_l2rus
/* 778 */     MCD_OPC_FilterValue, 191, 251, 3, 4, 0, // Skip to: 788
/* 784 */     MCD_OPC_Decode, 181, 1, 32, // Opcode: OUTPW_l2rus
/* 788 */     MCD_OPC_FilterValue, 223, 251, 3, 204, 0, // Skip to: 998
/* 794 */     MCD_OPC_Decode, 113, 32, // Opcode: INPW_l2rus
/* 797 */     MCD_OPC_FilterValue, 19, 12, 0, // Skip to: 813
/* 801 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 189, 0, // Skip to: 998
/* 809 */     MCD_OPC_Decode, 143, 1, 33, // Opcode: LDAWF_l2rus
/* 813 */     MCD_OPC_FilterValue, 20, 12, 0, // Skip to: 829
/* 817 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 173, 0, // Skip to: 998
/* 825 */     MCD_OPC_Decode, 136, 1, 33, // Opcode: LDAWB_l2rus
/* 829 */     MCD_OPC_FilterValue, 21, 11, 0, // Skip to: 844
/* 833 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 157, 0, // Skip to: 998
/* 841 */     MCD_OPC_Decode, 67, 34, // Opcode: CRC_l3r
/* 844 */     MCD_OPC_FilterValue, 24, 12, 0, // Skip to: 860
/* 848 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 142, 0, // Skip to: 998
/* 856 */     MCD_OPC_Decode, 186, 1, 21, // Opcode: REMS_l3r
/* 860 */     MCD_OPC_FilterValue, 25, 12, 0, // Skip to: 876
/* 864 */     MCD_OPC_CheckField, 11, 16, 159, 251, 3, 126, 0, // Skip to: 998
/* 872 */     MCD_OPC_Decode, 187, 1, 21, // Opcode: REMU_l3r
/* 876 */     MCD_OPC_FilterValue, 26, 29, 0, // Skip to: 909
/* 880 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 883 */     MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 896
/* 887 */     MCD_OPC_CheckField, 10, 6, 60, 105, 0, // Skip to: 998
/* 893 */     MCD_OPC_Decode, 39, 35, // Opcode: BLRF_lu10
/* 896 */     MCD_OPC_FilterValue, 1, 98, 0, // Skip to: 998
/* 900 */     MCD_OPC_CheckField, 10, 6, 60, 92, 0, // Skip to: 998
/* 906 */     MCD_OPC_Decode, 37, 36, // Opcode: BLRB_lu10
/* 909 */     MCD_OPC_FilterValue, 27, 31, 0, // Skip to: 944
/* 913 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 916 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 930
/* 920 */     MCD_OPC_CheckField, 10, 6, 60, 72, 0, // Skip to: 998
/* 926 */     MCD_OPC_Decode, 133, 1, 35, // Opcode: LDAPF_lu10
/* 930 */     MCD_OPC_FilterValue, 1, 64, 0, // Skip to: 998
/* 934 */     MCD_OPC_CheckField, 10, 6, 60, 58, 0, // Skip to: 998
/* 940 */     MCD_OPC_Decode, 131, 1, 36, // Opcode: LDAPB_lu10
/* 944 */     MCD_OPC_FilterValue, 28, 30, 0, // Skip to: 978
/* 948 */     MCD_OPC_ExtractField, 26, 1,  // Inst{26} ...
/* 951 */     MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 964
/* 955 */     MCD_OPC_CheckField, 10, 6, 60, 37, 0, // Skip to: 998
/* 961 */     MCD_OPC_Decode, 32, 35, // Opcode: BLACP_lu10
/* 964 */     MCD_OPC_FilterValue, 1, 30, 0, // Skip to: 998
/* 968 */     MCD_OPC_CheckField, 10, 6, 60, 24, 0, // Skip to: 998
/* 974 */     MCD_OPC_Decode, 155, 1, 35, // Opcode: LDWCP_lu10
/* 978 */     MCD_OPC_FilterValue, 29, 16, 0, // Skip to: 998
/* 982 */     MCD_OPC_CheckField, 26, 1, 0, 10, 0, // Skip to: 998
/* 988 */     MCD_OPC_CheckField, 10, 6, 60, 4, 0, // Skip to: 998
/* 994 */     MCD_OPC_Decode, 194, 1, 29, // Opcode: SETC_lru6
/* 998 */     MCD_OPC_Fail,
  0
};

static bool checkDecoderPredicate(unsigned Idx, uint64_t Bits)
{
  return true;  //llvm_unreachable("Invalid index!");
}

#define DecodeToMCInst(fname,fieldname, InsnType) \
static DecodeStatus fname(DecodeStatus S, unsigned Idx, InsnType insn, MCInst *MI, \
                uint64_t Address, const void *Decoder) \
{ \
  InsnType tmp; \
  switch (Idx) { \
  default: \
  case 0: \
    return S; \
  case 1: \
    tmp = fieldname(insn, 0, 4); \
    if (DecodeGRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 2: \
    if (Decode2RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 3: \
    if (Decode2RUSInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 4: \
    if (DecodeR2RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 5: \
    if (Decode3RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 6: \
    if (Decode2RImmInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 7: \
    if (Decode2RSrcDstInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 8: \
    if (DecodeRUSSrcDstBitpInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 9: \
    if (DecodeRUSInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 10: \
    tmp = fieldname(insn, 6, 4); \
    if (DecodeRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 6); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 11: \
    tmp = fieldname(insn, 0, 6); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 12: \
    tmp = fieldname(insn, 6, 4); \
    if (DecodeGRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 6); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 13: \
    tmp = fieldname(insn, 0, 6); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 14: \
    tmp = fieldname(insn, 6, 4); \
    if (DecodeGRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 6); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 15: \
    if (DecodeRUSBitpInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 16: \
    if (Decode2RUSBitpInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 17: \
    if (Decode3RImmInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 18: \
    tmp = fieldname(insn, 0, 10); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 19: \
    tmp = fieldname(insn, 0, 10); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 20: \
    if (DecodeL2RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 21: \
    if (DecodeL3RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 22: \
    if (DecodeL4RSrcDstInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 23: \
    if (DecodeL4RSrcDstSrcDstInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 24: \
    if (DecodeL5RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 25: \
    if (DecodeL6RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 26: \
    if (DecodeLR2RInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 27: \
    tmp = fieldname(insn, 22, 4); \
    if (DecodeRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 6); \
    tmp |= (fieldname(insn, 16, 6) << 0); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 28: \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 6); \
    tmp |= (fieldname(insn, 16, 6) << 0); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 29: \
    tmp = fieldname(insn, 22, 4); \
    if (DecodeGRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 6); \
    tmp |= (fieldname(insn, 16, 6) << 0); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 30: \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 6); \
    tmp |= (fieldname(insn, 16, 6) << 0); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 31: \
    tmp = fieldname(insn, 22, 4); \
    if (DecodeGRRegsRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 6); \
    tmp |= (fieldname(insn, 16, 6) << 0); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 32: \
    if (DecodeL2RUSBitpInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 33: \
    if (DecodeL2RUSInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 34: \
    if (DecodeL3RSrcDstInstruction(MI, insn, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 35: \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 10); \
    tmp |= (fieldname(insn, 16, 10) << 0); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 36: \
    tmp = 0; \
    tmp |= (fieldname(insn, 0, 10) << 10); \
    tmp |= (fieldname(insn, 16, 10) << 0); \
    if (DecodeNegImmOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
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


FieldFromInstruction(fieldFromInstruction_2, uint16_t)
DecodeToMCInst(decodeToMCInst_2, fieldFromInstruction_2, uint16_t)
DecodeInstruction(decodeInstruction_2, fieldFromInstruction_2, decodeToMCInst_2, uint16_t)
FieldFromInstruction(fieldFromInstruction_4, uint32_t)
DecodeToMCInst(decodeToMCInst_4, fieldFromInstruction_4, uint32_t)
DecodeInstruction(decodeInstruction_4, fieldFromInstruction_4, decodeToMCInst_4, uint32_t)
