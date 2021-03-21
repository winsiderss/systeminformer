/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* * PPC Disassembler                                                         *|
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
static InsnType fname(InsnType insn, unsigned startBit, \
                                     unsigned numBits) \
{ \
    InsnType fieldMask; \
    if (numBits == sizeof(InsnType)*8) \
      fieldMask = (InsnType)(-1LL); \
    else \
      fieldMask = (((InsnType)1 << numBits) - 1) << startBit; \
    return (insn & fieldMask) >> startBit; \
}

// FieldFromInstruction(fieldFromInstruction_2, uint16_t)
FieldFromInstruction(fieldFromInstruction_4, uint32_t)

static const uint8_t DecoderTable32[] = {
/* 0 */       MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 3 */       MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 17
/* 7 */       MCD_OPC_CheckField, 1, 10, 128, 2, 231, 38, // Skip to: 9973
/* 14 */      MCD_OPC_Decode, 119, 0, // Opcode: ATTN
/* 17 */      MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 25
/* 21 */      MCD_OPC_Decode, 247, 8, 1, // Opcode: TDI
/* 25 */      MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 33
/* 29 */      MCD_OPC_Decode, 136, 9, 2, // Opcode: TWI
/* 33 */      MCD_OPC_FilterValue, 4, 223, 6, // Skip to: 1796
/* 37 */      MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 40 */      MCD_OPC_FilterValue, 0, 179, 0, // Skip to: 223
/* 44 */      MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 47 */      MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 55
/* 51 */      MCD_OPC_Decode, 144, 9, 3, // Opcode: VADDUBM
/* 55 */      MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 63
/* 59 */      MCD_OPC_Decode, 147, 9, 3, // Opcode: VADDUHM
/* 63 */      MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 71
/* 67 */      MCD_OPC_Decode, 149, 9, 3, // Opcode: VADDUWM
/* 71 */      MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 79
/* 75 */      MCD_OPC_Decode, 146, 9, 3, // Opcode: VADDUDM
/* 79 */      MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 87
/* 83 */      MCD_OPC_Decode, 139, 9, 3, // Opcode: VADDCUW
/* 87 */      MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 95
/* 91 */      MCD_OPC_Decode, 145, 9, 3, // Opcode: VADDUBS
/* 95 */      MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 103
/* 99 */      MCD_OPC_Decode, 148, 9, 3, // Opcode: VADDUHS
/* 103 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 111
/* 107 */     MCD_OPC_Decode, 150, 9, 3, // Opcode: VADDUWS
/* 111 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 119
/* 115 */     MCD_OPC_Decode, 141, 9, 3, // Opcode: VADDSBS
/* 119 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 127
/* 123 */     MCD_OPC_Decode, 142, 9, 3, // Opcode: VADDSHS
/* 127 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 135
/* 131 */     MCD_OPC_Decode, 143, 9, 3, // Opcode: VADDSWS
/* 135 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 143
/* 139 */     MCD_OPC_Decode, 183, 10, 3, // Opcode: VSUBUBM
/* 143 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 151
/* 147 */     MCD_OPC_Decode, 186, 10, 3, // Opcode: VSUBUHM
/* 151 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 159
/* 155 */     MCD_OPC_Decode, 188, 10, 3, // Opcode: VSUBUWM
/* 159 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 167
/* 163 */     MCD_OPC_Decode, 185, 10, 3, // Opcode: VSUBUDM
/* 167 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 175
/* 171 */     MCD_OPC_Decode, 178, 10, 3, // Opcode: VSUBCUW
/* 175 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 183
/* 179 */     MCD_OPC_Decode, 184, 10, 3, // Opcode: VSUBUBS
/* 183 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 191
/* 187 */     MCD_OPC_Decode, 187, 10, 3, // Opcode: VSUBUHS
/* 191 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 199
/* 195 */     MCD_OPC_Decode, 189, 10, 3, // Opcode: VSUBUWS
/* 199 */     MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 207
/* 203 */     MCD_OPC_Decode, 180, 10, 3, // Opcode: VSUBSBS
/* 207 */     MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 215
/* 211 */     MCD_OPC_Decode, 181, 10, 3, // Opcode: VSUBSHS
/* 215 */     MCD_OPC_FilterValue, 30, 26, 38, // Skip to: 9973
/* 219 */     MCD_OPC_Decode, 182, 10, 3, // Opcode: VSUBSWS
/* 223 */     MCD_OPC_FilterValue, 2, 235, 0, // Skip to: 462
/* 227 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 230 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 238
/* 234 */     MCD_OPC_Decode, 212, 9, 3, // Opcode: VMAXUB
/* 238 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 246
/* 242 */     MCD_OPC_Decode, 214, 9, 3, // Opcode: VMAXUH
/* 246 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 254
/* 250 */     MCD_OPC_Decode, 215, 9, 3, // Opcode: VMAXUW
/* 254 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 262
/* 258 */     MCD_OPC_Decode, 213, 9, 3, // Opcode: VMAXUD
/* 262 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 270
/* 266 */     MCD_OPC_Decode, 208, 9, 3, // Opcode: VMAXSB
/* 270 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 278
/* 274 */     MCD_OPC_Decode, 210, 9, 3, // Opcode: VMAXSH
/* 278 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 286
/* 282 */     MCD_OPC_Decode, 211, 9, 3, // Opcode: VMAXSW
/* 286 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 294
/* 290 */     MCD_OPC_Decode, 209, 9, 3, // Opcode: VMAXSD
/* 294 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 302
/* 298 */     MCD_OPC_Decode, 224, 9, 3, // Opcode: VMINUB
/* 302 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 310
/* 306 */     MCD_OPC_Decode, 225, 9, 3, // Opcode: VMINUH
/* 310 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 318
/* 314 */     MCD_OPC_Decode, 226, 9, 3, // Opcode: VMINUW
/* 318 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 326
/* 322 */     MCD_OPC_Decode, 218, 9, 3, // Opcode: VMIDUD
/* 326 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 334
/* 330 */     MCD_OPC_Decode, 220, 9, 3, // Opcode: VMINSB
/* 334 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 342
/* 338 */     MCD_OPC_Decode, 222, 9, 3, // Opcode: VMINSH
/* 342 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 350
/* 346 */     MCD_OPC_Decode, 223, 9, 3, // Opcode: VMINSW
/* 350 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 358
/* 354 */     MCD_OPC_Decode, 221, 9, 3, // Opcode: VMINSD
/* 358 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 366
/* 362 */     MCD_OPC_Decode, 156, 9, 3, // Opcode: VAVGUB
/* 366 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 374
/* 370 */     MCD_OPC_Decode, 157, 9, 3, // Opcode: VAVGUH
/* 374 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 382
/* 378 */     MCD_OPC_Decode, 158, 9, 3, // Opcode: VAVGUW
/* 382 */     MCD_OPC_FilterValue, 20, 4, 0, // Skip to: 390
/* 386 */     MCD_OPC_Decode, 153, 9, 3, // Opcode: VAVGSB
/* 390 */     MCD_OPC_FilterValue, 21, 4, 0, // Skip to: 398
/* 394 */     MCD_OPC_Decode, 154, 9, 3, // Opcode: VAVGSH
/* 398 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 406
/* 402 */     MCD_OPC_Decode, 155, 9, 3, // Opcode: VAVGSW
/* 406 */     MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 420
/* 410 */     MCD_OPC_CheckField, 16, 5, 0, 85, 37, // Skip to: 9973
/* 416 */     MCD_OPC_Decode, 163, 9, 4, // Opcode: VCLZB
/* 420 */     MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 434
/* 424 */     MCD_OPC_CheckField, 16, 5, 0, 71, 37, // Skip to: 9973
/* 430 */     MCD_OPC_Decode, 165, 9, 4, // Opcode: VCLZH
/* 434 */     MCD_OPC_FilterValue, 30, 10, 0, // Skip to: 448
/* 438 */     MCD_OPC_CheckField, 16, 5, 0, 57, 37, // Skip to: 9973
/* 444 */     MCD_OPC_Decode, 166, 9, 4, // Opcode: VCLZW
/* 448 */     MCD_OPC_FilterValue, 31, 49, 37, // Skip to: 9973
/* 452 */     MCD_OPC_CheckField, 16, 5, 0, 43, 37, // Skip to: 9973
/* 458 */     MCD_OPC_Decode, 164, 9, 4, // Opcode: VCLZD
/* 462 */     MCD_OPC_FilterValue, 3, 59, 0, // Skip to: 525
/* 466 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 469 */     MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 483
/* 473 */     MCD_OPC_CheckField, 16, 5, 0, 22, 37, // Skip to: 9973
/* 479 */     MCD_OPC_Decode, 140, 10, 4, // Opcode: VPOPCNTB
/* 483 */     MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 497
/* 487 */     MCD_OPC_CheckField, 16, 5, 0, 8, 37, // Skip to: 9973
/* 493 */     MCD_OPC_Decode, 142, 10, 4, // Opcode: VPOPCNTH
/* 497 */     MCD_OPC_FilterValue, 30, 10, 0, // Skip to: 511
/* 501 */     MCD_OPC_CheckField, 16, 5, 0, 250, 36, // Skip to: 9973
/* 507 */     MCD_OPC_Decode, 143, 10, 4, // Opcode: VPOPCNTW
/* 511 */     MCD_OPC_FilterValue, 31, 242, 36, // Skip to: 9973
/* 515 */     MCD_OPC_CheckField, 16, 5, 0, 236, 36, // Skip to: 9973
/* 521 */     MCD_OPC_Decode, 141, 10, 4, // Opcode: VPOPCNTD
/* 525 */     MCD_OPC_FilterValue, 4, 239, 0, // Skip to: 768
/* 529 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 532 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 540
/* 536 */     MCD_OPC_Decode, 149, 10, 3, // Opcode: VRLB
/* 540 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 548
/* 544 */     MCD_OPC_Decode, 151, 10, 3, // Opcode: VRLH
/* 548 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 556
/* 552 */     MCD_OPC_Decode, 152, 10, 3, // Opcode: VRLW
/* 556 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 564
/* 560 */     MCD_OPC_Decode, 150, 10, 3, // Opcode: VRLD
/* 564 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 572
/* 568 */     MCD_OPC_Decode, 156, 10, 3, // Opcode: VSLB
/* 572 */     MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 580
/* 576 */     MCD_OPC_Decode, 159, 10, 3, // Opcode: VSLH
/* 580 */     MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 588
/* 584 */     MCD_OPC_Decode, 161, 10, 3, // Opcode: VSLW
/* 588 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 596
/* 592 */     MCD_OPC_Decode, 155, 10, 3, // Opcode: VSL
/* 596 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 604
/* 600 */     MCD_OPC_Decode, 173, 10, 3, // Opcode: VSRB
/* 604 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 612
/* 608 */     MCD_OPC_Decode, 175, 10, 3, // Opcode: VSRH
/* 612 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 620
/* 616 */     MCD_OPC_Decode, 177, 10, 3, // Opcode: VSRW
/* 620 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 628
/* 624 */     MCD_OPC_Decode, 168, 10, 3, // Opcode: VSR
/* 628 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 636
/* 632 */     MCD_OPC_Decode, 169, 10, 3, // Opcode: VSRAB
/* 636 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 644
/* 640 */     MCD_OPC_Decode, 171, 10, 3, // Opcode: VSRAH
/* 644 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 652
/* 648 */     MCD_OPC_Decode, 172, 10, 3, // Opcode: VSRAW
/* 652 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 660
/* 656 */     MCD_OPC_Decode, 170, 10, 3, // Opcode: VSRAD
/* 660 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 668
/* 664 */     MCD_OPC_Decode, 151, 9, 3, // Opcode: VAND
/* 668 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 676
/* 672 */     MCD_OPC_Decode, 152, 9, 3, // Opcode: VANDC
/* 676 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 684
/* 680 */     MCD_OPC_Decode, 128, 10, 3, // Opcode: VOR
/* 684 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 692
/* 688 */     MCD_OPC_Decode, 201, 10, 3, // Opcode: VXOR
/* 692 */     MCD_OPC_FilterValue, 20, 4, 0, // Skip to: 700
/* 696 */     MCD_OPC_Decode, 255, 9, 3, // Opcode: VNOR
/* 700 */     MCD_OPC_FilterValue, 21, 4, 0, // Skip to: 708
/* 704 */     MCD_OPC_Decode, 129, 10, 3, // Opcode: VORC
/* 708 */     MCD_OPC_FilterValue, 22, 4, 0, // Skip to: 716
/* 712 */     MCD_OPC_Decode, 253, 9, 3, // Opcode: VNAND
/* 716 */     MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 724
/* 720 */     MCD_OPC_Decode, 157, 10, 3, // Opcode: VSLD
/* 724 */     MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 738
/* 728 */     MCD_OPC_CheckField, 11, 10, 0, 23, 36, // Skip to: 9973
/* 734 */     MCD_OPC_Decode, 198, 5, 5, // Opcode: MFVSCR
/* 738 */     MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 752
/* 742 */     MCD_OPC_CheckField, 16, 10, 0, 9, 36, // Skip to: 9973
/* 748 */     MCD_OPC_Decode, 225, 5, 6, // Opcode: MTVSCR
/* 752 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 760
/* 756 */     MCD_OPC_Decode, 203, 9, 3, // Opcode: VEQV
/* 760 */     MCD_OPC_FilterValue, 27, 249, 35, // Skip to: 9973
/* 764 */     MCD_OPC_Decode, 174, 10, 3, // Opcode: VSRD
/* 768 */     MCD_OPC_FilterValue, 6, 211, 0, // Skip to: 983
/* 772 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 775 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 783
/* 779 */     MCD_OPC_Decode, 171, 9, 3, // Opcode: VCMPEQUB
/* 783 */     MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 791
/* 787 */     MCD_OPC_Decode, 175, 9, 3, // Opcode: VCMPEQUH
/* 791 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 799
/* 795 */     MCD_OPC_Decode, 177, 9, 3, // Opcode: VCMPEQUW
/* 799 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 807
/* 803 */     MCD_OPC_Decode, 169, 9, 3, // Opcode: VCMPEQFP
/* 807 */     MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 815
/* 811 */     MCD_OPC_Decode, 179, 9, 3, // Opcode: VCMPGEFP
/* 815 */     MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 823
/* 819 */     MCD_OPC_Decode, 191, 9, 3, // Opcode: VCMPGTUB
/* 823 */     MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 831
/* 827 */     MCD_OPC_Decode, 195, 9, 3, // Opcode: VCMPGTUH
/* 831 */     MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 839
/* 835 */     MCD_OPC_Decode, 197, 9, 3, // Opcode: VCMPGTUW
/* 839 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 847
/* 843 */     MCD_OPC_Decode, 181, 9, 3, // Opcode: VCMPGTFP
/* 847 */     MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 855
/* 851 */     MCD_OPC_Decode, 183, 9, 3, // Opcode: VCMPGTSB
/* 855 */     MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 863
/* 859 */     MCD_OPC_Decode, 187, 9, 3, // Opcode: VCMPGTSH
/* 863 */     MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 871
/* 867 */     MCD_OPC_Decode, 189, 9, 3, // Opcode: VCMPGTSW
/* 871 */     MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 879
/* 875 */     MCD_OPC_Decode, 167, 9, 3, // Opcode: VCMPBFP
/* 879 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 887
/* 883 */     MCD_OPC_Decode, 172, 9, 3, // Opcode: VCMPEQUBo
/* 887 */     MCD_OPC_FilterValue, 17, 4, 0, // Skip to: 895
/* 891 */     MCD_OPC_Decode, 176, 9, 3, // Opcode: VCMPEQUHo
/* 895 */     MCD_OPC_FilterValue, 18, 4, 0, // Skip to: 903
/* 899 */     MCD_OPC_Decode, 178, 9, 3, // Opcode: VCMPEQUWo
/* 903 */     MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 911
/* 907 */     MCD_OPC_Decode, 170, 9, 3, // Opcode: VCMPEQFPo
/* 911 */     MCD_OPC_FilterValue, 23, 4, 0, // Skip to: 919
/* 915 */     MCD_OPC_Decode, 180, 9, 3, // Opcode: VCMPGEFPo
/* 919 */     MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 927
/* 923 */     MCD_OPC_Decode, 192, 9, 3, // Opcode: VCMPGTUBo
/* 927 */     MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 935
/* 931 */     MCD_OPC_Decode, 196, 9, 3, // Opcode: VCMPGTUHo
/* 935 */     MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 943
/* 939 */     MCD_OPC_Decode, 198, 9, 3, // Opcode: VCMPGTUWo
/* 943 */     MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 951
/* 947 */     MCD_OPC_Decode, 182, 9, 3, // Opcode: VCMPGTFPo
/* 951 */     MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 959
/* 955 */     MCD_OPC_Decode, 184, 9, 3, // Opcode: VCMPGTSBo
/* 959 */     MCD_OPC_FilterValue, 29, 4, 0, // Skip to: 967
/* 963 */     MCD_OPC_Decode, 188, 9, 3, // Opcode: VCMPGTSHo
/* 967 */     MCD_OPC_FilterValue, 30, 4, 0, // Skip to: 975
/* 971 */     MCD_OPC_Decode, 190, 9, 3, // Opcode: VCMPGTSWo
/* 975 */     MCD_OPC_FilterValue, 31, 34, 35, // Skip to: 9973
/* 979 */     MCD_OPC_Decode, 168, 9, 3, // Opcode: VCMPBFPo
/* 983 */     MCD_OPC_FilterValue, 7, 51, 0, // Skip to: 1038
/* 987 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 990 */     MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 998
/* 994 */     MCD_OPC_Decode, 173, 9, 3, // Opcode: VCMPEQUD
/* 998 */     MCD_OPC_FilterValue, 11, 4, 0, // Skip to: 1006
/* 1002 */    MCD_OPC_Decode, 193, 9, 3, // Opcode: VCMPGTUD
/* 1006 */    MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 1014
/* 1010 */    MCD_OPC_Decode, 185, 9, 3, // Opcode: VCMPGTSD
/* 1014 */    MCD_OPC_FilterValue, 19, 4, 0, // Skip to: 1022
/* 1018 */    MCD_OPC_Decode, 174, 9, 3, // Opcode: VCMPEQUDo
/* 1022 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 1030
/* 1026 */    MCD_OPC_Decode, 194, 9, 3, // Opcode: VCMPGTUDo
/* 1030 */    MCD_OPC_FilterValue, 31, 235, 34, // Skip to: 9973
/* 1034 */    MCD_OPC_Decode, 186, 9, 3, // Opcode: VCMPGTSDo
/* 1038 */    MCD_OPC_FilterValue, 8, 139, 0, // Skip to: 1181
/* 1042 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1045 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1053
/* 1049 */    MCD_OPC_Decode, 249, 9, 3, // Opcode: VMULOUB
/* 1053 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1061
/* 1057 */    MCD_OPC_Decode, 250, 9, 3, // Opcode: VMULOUH
/* 1061 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 1069
/* 1065 */    MCD_OPC_Decode, 251, 9, 3, // Opcode: VMULOUW
/* 1069 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1077
/* 1073 */    MCD_OPC_Decode, 246, 9, 3, // Opcode: VMULOSB
/* 1077 */    MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 1085
/* 1081 */    MCD_OPC_Decode, 247, 9, 3, // Opcode: VMULOSH
/* 1085 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 1093
/* 1089 */    MCD_OPC_Decode, 248, 9, 3, // Opcode: VMULOSW
/* 1093 */    MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 1101
/* 1097 */    MCD_OPC_Decode, 243, 9, 3, // Opcode: VMULEUB
/* 1101 */    MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 1109
/* 1105 */    MCD_OPC_Decode, 244, 9, 3, // Opcode: VMULEUH
/* 1109 */    MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 1117
/* 1113 */    MCD_OPC_Decode, 245, 9, 3, // Opcode: VMULEUW
/* 1117 */    MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 1125
/* 1121 */    MCD_OPC_Decode, 240, 9, 3, // Opcode: VMULESB
/* 1125 */    MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 1133
/* 1129 */    MCD_OPC_Decode, 241, 9, 3, // Opcode: VMULESH
/* 1133 */    MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 1141
/* 1137 */    MCD_OPC_Decode, 242, 9, 3, // Opcode: VMULESW
/* 1141 */    MCD_OPC_FilterValue, 24, 4, 0, // Skip to: 1149
/* 1145 */    MCD_OPC_Decode, 193, 10, 3, // Opcode: VSUM4UBS
/* 1149 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 1157
/* 1153 */    MCD_OPC_Decode, 192, 10, 3, // Opcode: VSUM4SHS
/* 1157 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 1165
/* 1161 */    MCD_OPC_Decode, 190, 10, 3, // Opcode: VSUM2SWS
/* 1165 */    MCD_OPC_FilterValue, 28, 4, 0, // Skip to: 1173
/* 1169 */    MCD_OPC_Decode, 191, 10, 3, // Opcode: VSUM4SBS
/* 1173 */    MCD_OPC_FilterValue, 30, 92, 34, // Skip to: 9973
/* 1177 */    MCD_OPC_Decode, 194, 10, 3, // Opcode: VSUMSWS
/* 1181 */    MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 1195
/* 1185 */    MCD_OPC_CheckField, 6, 5, 2, 78, 34, // Skip to: 9973
/* 1191 */    MCD_OPC_Decode, 252, 9, 3, // Opcode: VMULUWM
/* 1195 */    MCD_OPC_FilterValue, 10, 179, 0, // Skip to: 1378
/* 1199 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1202 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1210
/* 1206 */    MCD_OPC_Decode, 140, 9, 3, // Opcode: VADDFP
/* 1210 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1218
/* 1214 */    MCD_OPC_Decode, 179, 10, 3, // Opcode: VSUBFP
/* 1218 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 1232
/* 1222 */    MCD_OPC_CheckField, 16, 5, 0, 41, 34, // Skip to: 9973
/* 1228 */    MCD_OPC_Decode, 144, 10, 4, // Opcode: VREFP
/* 1232 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 1246
/* 1236 */    MCD_OPC_CheckField, 16, 5, 0, 27, 34, // Skip to: 9973
/* 1242 */    MCD_OPC_Decode, 153, 10, 4, // Opcode: VRSQRTEFP
/* 1246 */    MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 1260
/* 1250 */    MCD_OPC_CheckField, 16, 5, 0, 13, 34, // Skip to: 9973
/* 1256 */    MCD_OPC_Decode, 204, 9, 4, // Opcode: VEXPTEFP
/* 1260 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 1274
/* 1264 */    MCD_OPC_CheckField, 16, 5, 0, 255, 33, // Skip to: 9973
/* 1270 */    MCD_OPC_Decode, 205, 9, 4, // Opcode: VLOGEFP
/* 1274 */    MCD_OPC_FilterValue, 8, 10, 0, // Skip to: 1288
/* 1278 */    MCD_OPC_CheckField, 16, 5, 0, 241, 33, // Skip to: 9973
/* 1284 */    MCD_OPC_Decode, 146, 10, 4, // Opcode: VRFIN
/* 1288 */    MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 1302
/* 1292 */    MCD_OPC_CheckField, 16, 5, 0, 227, 33, // Skip to: 9973
/* 1298 */    MCD_OPC_Decode, 148, 10, 4, // Opcode: VRFIZ
/* 1302 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 1316
/* 1306 */    MCD_OPC_CheckField, 16, 5, 0, 213, 33, // Skip to: 9973
/* 1312 */    MCD_OPC_Decode, 147, 10, 4, // Opcode: VRFIP
/* 1316 */    MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 1330
/* 1320 */    MCD_OPC_CheckField, 16, 5, 0, 199, 33, // Skip to: 9973
/* 1326 */    MCD_OPC_Decode, 145, 10, 4, // Opcode: VRFIM
/* 1330 */    MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 1338
/* 1334 */    MCD_OPC_Decode, 161, 9, 7, // Opcode: VCFUX
/* 1338 */    MCD_OPC_FilterValue, 13, 4, 0, // Skip to: 1346
/* 1342 */    MCD_OPC_Decode, 159, 9, 7, // Opcode: VCFSX
/* 1346 */    MCD_OPC_FilterValue, 14, 4, 0, // Skip to: 1354
/* 1350 */    MCD_OPC_Decode, 201, 9, 7, // Opcode: VCTUXS
/* 1354 */    MCD_OPC_FilterValue, 15, 4, 0, // Skip to: 1362
/* 1358 */    MCD_OPC_Decode, 199, 9, 7, // Opcode: VCTSXS
/* 1362 */    MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 1370
/* 1366 */    MCD_OPC_Decode, 207, 9, 3, // Opcode: VMAXFP
/* 1370 */    MCD_OPC_FilterValue, 17, 151, 33, // Skip to: 9973
/* 1374 */    MCD_OPC_Decode, 219, 9, 3, // Opcode: VMINFP
/* 1378 */    MCD_OPC_FilterValue, 12, 133, 0, // Skip to: 1515
/* 1382 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1385 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1393
/* 1389 */    MCD_OPC_Decode, 228, 9, 3, // Opcode: VMRGHB
/* 1393 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1401
/* 1397 */    MCD_OPC_Decode, 229, 9, 3, // Opcode: VMRGHH
/* 1401 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 1409
/* 1405 */    MCD_OPC_Decode, 230, 9, 3, // Opcode: VMRGHW
/* 1409 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1417
/* 1413 */    MCD_OPC_Decode, 231, 9, 3, // Opcode: VMRGLB
/* 1417 */    MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 1425
/* 1421 */    MCD_OPC_Decode, 232, 9, 3, // Opcode: VMRGLH
/* 1425 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 1433
/* 1429 */    MCD_OPC_Decode, 233, 9, 3, // Opcode: VMRGLW
/* 1433 */    MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 1441
/* 1437 */    MCD_OPC_Decode, 162, 10, 7, // Opcode: VSPLTB
/* 1441 */    MCD_OPC_FilterValue, 9, 4, 0, // Skip to: 1449
/* 1445 */    MCD_OPC_Decode, 163, 10, 7, // Opcode: VSPLTH
/* 1449 */    MCD_OPC_FilterValue, 10, 4, 0, // Skip to: 1457
/* 1453 */    MCD_OPC_Decode, 167, 10, 7, // Opcode: VSPLTW
/* 1457 */    MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 1471
/* 1461 */    MCD_OPC_CheckField, 11, 5, 0, 58, 33, // Skip to: 9973
/* 1467 */    MCD_OPC_Decode, 164, 10, 8, // Opcode: VSPLTISB
/* 1471 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 1485
/* 1475 */    MCD_OPC_CheckField, 11, 5, 0, 44, 33, // Skip to: 9973
/* 1481 */    MCD_OPC_Decode, 165, 10, 8, // Opcode: VSPLTISH
/* 1485 */    MCD_OPC_FilterValue, 14, 10, 0, // Skip to: 1499
/* 1489 */    MCD_OPC_CheckField, 11, 5, 0, 30, 33, // Skip to: 9973
/* 1495 */    MCD_OPC_Decode, 166, 10, 8, // Opcode: VSPLTISW
/* 1499 */    MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 1507
/* 1503 */    MCD_OPC_Decode, 160, 10, 3, // Opcode: VSLO
/* 1507 */    MCD_OPC_FilterValue, 17, 14, 33, // Skip to: 9973
/* 1511 */    MCD_OPC_Decode, 176, 10, 3, // Opcode: VSRO
/* 1515 */    MCD_OPC_FilterValue, 14, 159, 0, // Skip to: 1678
/* 1519 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1522 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1530
/* 1526 */    MCD_OPC_Decode, 136, 10, 3, // Opcode: VPKUHUM
/* 1530 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 1538
/* 1534 */    MCD_OPC_Decode, 138, 10, 3, // Opcode: VPKUWUM
/* 1538 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 1546
/* 1542 */    MCD_OPC_Decode, 137, 10, 3, // Opcode: VPKUHUS
/* 1546 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 1554
/* 1550 */    MCD_OPC_Decode, 139, 10, 3, // Opcode: VPKUWUS
/* 1554 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1562
/* 1558 */    MCD_OPC_Decode, 133, 10, 3, // Opcode: VPKSHUS
/* 1562 */    MCD_OPC_FilterValue, 5, 4, 0, // Skip to: 1570
/* 1566 */    MCD_OPC_Decode, 135, 10, 3, // Opcode: VPKSWUS
/* 1570 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 1578
/* 1574 */    MCD_OPC_Decode, 132, 10, 3, // Opcode: VPKSHSS
/* 1578 */    MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 1586
/* 1582 */    MCD_OPC_Decode, 134, 10, 3, // Opcode: VPKSWSS
/* 1586 */    MCD_OPC_FilterValue, 8, 10, 0, // Skip to: 1600
/* 1590 */    MCD_OPC_CheckField, 16, 5, 0, 185, 32, // Skip to: 9973
/* 1596 */    MCD_OPC_Decode, 196, 10, 4, // Opcode: VUPKHSB
/* 1600 */    MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 1614
/* 1604 */    MCD_OPC_CheckField, 16, 5, 0, 171, 32, // Skip to: 9973
/* 1610 */    MCD_OPC_Decode, 197, 10, 4, // Opcode: VUPKHSH
/* 1614 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 1628
/* 1618 */    MCD_OPC_CheckField, 16, 5, 0, 157, 32, // Skip to: 9973
/* 1624 */    MCD_OPC_Decode, 199, 10, 4, // Opcode: VUPKLSB
/* 1628 */    MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 1642
/* 1632 */    MCD_OPC_CheckField, 16, 5, 0, 143, 32, // Skip to: 9973
/* 1638 */    MCD_OPC_Decode, 200, 10, 4, // Opcode: VUPKLSH
/* 1642 */    MCD_OPC_FilterValue, 12, 4, 0, // Skip to: 1650
/* 1646 */    MCD_OPC_Decode, 131, 10, 3, // Opcode: VPKPX
/* 1650 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 1664
/* 1654 */    MCD_OPC_CheckField, 16, 5, 0, 121, 32, // Skip to: 9973
/* 1660 */    MCD_OPC_Decode, 195, 10, 4, // Opcode: VUPKHPX
/* 1664 */    MCD_OPC_FilterValue, 15, 113, 32, // Skip to: 9973
/* 1668 */    MCD_OPC_CheckField, 16, 5, 0, 107, 32, // Skip to: 9973
/* 1674 */    MCD_OPC_Decode, 198, 10, 4, // Opcode: VUPKLPX
/* 1678 */    MCD_OPC_FilterValue, 32, 4, 0, // Skip to: 1686
/* 1682 */    MCD_OPC_Decode, 216, 9, 9, // Opcode: VMHADDSHS
/* 1686 */    MCD_OPC_FilterValue, 33, 4, 0, // Skip to: 1694
/* 1690 */    MCD_OPC_Decode, 217, 9, 9, // Opcode: VMHRADDSHS
/* 1694 */    MCD_OPC_FilterValue, 34, 4, 0, // Skip to: 1702
/* 1698 */    MCD_OPC_Decode, 227, 9, 9, // Opcode: VMLADDUHM
/* 1702 */    MCD_OPC_FilterValue, 36, 4, 0, // Skip to: 1710
/* 1706 */    MCD_OPC_Decode, 237, 9, 9, // Opcode: VMSUMUBM
/* 1710 */    MCD_OPC_FilterValue, 37, 4, 0, // Skip to: 1718
/* 1714 */    MCD_OPC_Decode, 234, 9, 9, // Opcode: VMSUMMBM
/* 1718 */    MCD_OPC_FilterValue, 38, 4, 0, // Skip to: 1726
/* 1722 */    MCD_OPC_Decode, 238, 9, 9, // Opcode: VMSUMUHM
/* 1726 */    MCD_OPC_FilterValue, 39, 4, 0, // Skip to: 1734
/* 1730 */    MCD_OPC_Decode, 239, 9, 9, // Opcode: VMSUMUHS
/* 1734 */    MCD_OPC_FilterValue, 40, 4, 0, // Skip to: 1742
/* 1738 */    MCD_OPC_Decode, 235, 9, 9, // Opcode: VMSUMSHM
/* 1742 */    MCD_OPC_FilterValue, 41, 4, 0, // Skip to: 1750
/* 1746 */    MCD_OPC_Decode, 236, 9, 9, // Opcode: VMSUMSHS
/* 1750 */    MCD_OPC_FilterValue, 42, 4, 0, // Skip to: 1758
/* 1754 */    MCD_OPC_Decode, 154, 10, 9, // Opcode: VSEL
/* 1758 */    MCD_OPC_FilterValue, 43, 4, 0, // Skip to: 1766
/* 1762 */    MCD_OPC_Decode, 130, 10, 9, // Opcode: VPERM
/* 1766 */    MCD_OPC_FilterValue, 44, 10, 0, // Skip to: 1780
/* 1770 */    MCD_OPC_CheckField, 10, 1, 0, 5, 32, // Skip to: 9973
/* 1776 */    MCD_OPC_Decode, 158, 10, 10, // Opcode: VSLDOI
/* 1780 */    MCD_OPC_FilterValue, 46, 4, 0, // Skip to: 1788
/* 1784 */    MCD_OPC_Decode, 206, 9, 11, // Opcode: VMADDFP
/* 1788 */    MCD_OPC_FilterValue, 47, 245, 31, // Skip to: 9973
/* 1792 */    MCD_OPC_Decode, 254, 9, 11, // Opcode: VNMSUBFP
/* 1796 */    MCD_OPC_FilterValue, 7, 4, 0, // Skip to: 1804
/* 1800 */    MCD_OPC_Decode, 236, 5, 12, // Opcode: MULLI
/* 1804 */    MCD_OPC_FilterValue, 8, 4, 0, // Skip to: 1812
/* 1808 */    MCD_OPC_Decode, 218, 8, 12, // Opcode: SUBFIC
/* 1812 */    MCD_OPC_FilterValue, 10, 19, 0, // Skip to: 1835
/* 1816 */    MCD_OPC_ExtractField, 21, 2,  // Inst{22-21} ...
/* 1819 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1827
/* 1823 */    MCD_OPC_Decode, 223, 1, 13, // Opcode: CMPLWI
/* 1827 */    MCD_OPC_FilterValue, 1, 206, 31, // Skip to: 9973
/* 1831 */    MCD_OPC_Decode, 221, 1, 14, // Opcode: CMPLDI
/* 1835 */    MCD_OPC_FilterValue, 11, 19, 0, // Skip to: 1858
/* 1839 */    MCD_OPC_ExtractField, 21, 2,  // Inst{22-21} ...
/* 1842 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1850
/* 1846 */    MCD_OPC_Decode, 225, 1, 15, // Opcode: CMPWI
/* 1850 */    MCD_OPC_FilterValue, 1, 183, 31, // Skip to: 9973
/* 1854 */    MCD_OPC_Decode, 219, 1, 16, // Opcode: CMPDI
/* 1858 */    MCD_OPC_FilterValue, 12, 3, 0, // Skip to: 1865
/* 1862 */    MCD_OPC_Decode, 39, 12, // Opcode: ADDIC
/* 1865 */    MCD_OPC_FilterValue, 13, 3, 0, // Skip to: 1872
/* 1869 */    MCD_OPC_Decode, 41, 12, // Opcode: ADDICo
/* 1872 */    MCD_OPC_FilterValue, 14, 13, 0, // Skip to: 1889
/* 1876 */    MCD_OPC_CheckField, 16, 5, 0, 4, 0, // Skip to: 1886
/* 1882 */    MCD_OPC_Decode, 141, 5, 17, // Opcode: LI
/* 1886 */    MCD_OPC_Decode, 37, 18, // Opcode: ADDI
/* 1889 */    MCD_OPC_FilterValue, 15, 13, 0, // Skip to: 1906
/* 1893 */    MCD_OPC_CheckField, 16, 5, 0, 4, 0, // Skip to: 1903
/* 1899 */    MCD_OPC_Decode, 143, 5, 17, // Opcode: LIS
/* 1903 */    MCD_OPC_Decode, 42, 18, // Opcode: ADDIS
/* 1906 */    MCD_OPC_FilterValue, 16, 7, 1, // Skip to: 2173
/* 1910 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 1913 */    MCD_OPC_FilterValue, 0, 61, 0, // Skip to: 1978
/* 1917 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 1920 */    MCD_OPC_FilterValue, 128, 4, 4, 0, // Skip to: 1929
/* 1925 */    MCD_OPC_Decode, 154, 1, 19, // Opcode: BDNZ
/* 1929 */    MCD_OPC_FilterValue, 192, 4, 4, 0, // Skip to: 1938
/* 1934 */    MCD_OPC_Decode, 174, 1, 19, // Opcode: BDZ
/* 1938 */    MCD_OPC_FilterValue, 128, 6, 4, 0, // Skip to: 1947
/* 1943 */    MCD_OPC_Decode, 172, 1, 19, // Opcode: BDNZm
/* 1947 */    MCD_OPC_FilterValue, 160, 6, 4, 0, // Skip to: 1956
/* 1952 */    MCD_OPC_Decode, 173, 1, 19, // Opcode: BDNZp
/* 1956 */    MCD_OPC_FilterValue, 192, 6, 4, 0, // Skip to: 1965
/* 1961 */    MCD_OPC_Decode, 192, 1, 19, // Opcode: BDZm
/* 1965 */    MCD_OPC_FilterValue, 224, 6, 4, 0, // Skip to: 1974
/* 1970 */    MCD_OPC_Decode, 193, 1, 19, // Opcode: BDZp
/* 1974 */    MCD_OPC_Decode, 230, 11, 20, // Opcode: gBC
/* 1978 */    MCD_OPC_FilterValue, 1, 61, 0, // Skip to: 2043
/* 1982 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 1985 */    MCD_OPC_FilterValue, 128, 4, 4, 0, // Skip to: 1994
/* 1990 */    MCD_OPC_Decode, 159, 1, 19, // Opcode: BDNZL
/* 1994 */    MCD_OPC_FilterValue, 192, 4, 4, 0, // Skip to: 2003
/* 1999 */    MCD_OPC_Decode, 179, 1, 19, // Opcode: BDZL
/* 2003 */    MCD_OPC_FilterValue, 128, 6, 4, 0, // Skip to: 2012
/* 2008 */    MCD_OPC_Decode, 170, 1, 19, // Opcode: BDNZLm
/* 2012 */    MCD_OPC_FilterValue, 160, 6, 4, 0, // Skip to: 2021
/* 2017 */    MCD_OPC_Decode, 171, 1, 19, // Opcode: BDNZLp
/* 2021 */    MCD_OPC_FilterValue, 192, 6, 4, 0, // Skip to: 2030
/* 2026 */    MCD_OPC_Decode, 190, 1, 19, // Opcode: BDZLm
/* 2030 */    MCD_OPC_FilterValue, 224, 6, 4, 0, // Skip to: 2039
/* 2035 */    MCD_OPC_Decode, 191, 1, 19, // Opcode: BDZLp
/* 2039 */    MCD_OPC_Decode, 234, 11, 20, // Opcode: gBCL
/* 2043 */    MCD_OPC_FilterValue, 2, 61, 0, // Skip to: 2108
/* 2047 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 2050 */    MCD_OPC_FilterValue, 128, 4, 4, 0, // Skip to: 2059
/* 2055 */    MCD_OPC_Decode, 156, 1, 19, // Opcode: BDNZA
/* 2059 */    MCD_OPC_FilterValue, 192, 4, 4, 0, // Skip to: 2068
/* 2064 */    MCD_OPC_Decode, 176, 1, 19, // Opcode: BDZA
/* 2068 */    MCD_OPC_FilterValue, 128, 6, 4, 0, // Skip to: 2077
/* 2073 */    MCD_OPC_Decode, 157, 1, 19, // Opcode: BDNZAm
/* 2077 */    MCD_OPC_FilterValue, 160, 6, 4, 0, // Skip to: 2086
/* 2082 */    MCD_OPC_Decode, 158, 1, 19, // Opcode: BDNZAp
/* 2086 */    MCD_OPC_FilterValue, 192, 6, 4, 0, // Skip to: 2095
/* 2091 */    MCD_OPC_Decode, 177, 1, 19, // Opcode: BDZAm
/* 2095 */    MCD_OPC_FilterValue, 224, 6, 4, 0, // Skip to: 2104
/* 2100 */    MCD_OPC_Decode, 178, 1, 19, // Opcode: BDZAp
/* 2104 */    MCD_OPC_Decode, 231, 11, 20, // Opcode: gBCA
/* 2108 */    MCD_OPC_FilterValue, 3, 181, 30, // Skip to: 9973
/* 2112 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 2115 */    MCD_OPC_FilterValue, 128, 4, 4, 0, // Skip to: 2124
/* 2120 */    MCD_OPC_Decode, 160, 1, 19, // Opcode: BDNZLA
/* 2124 */    MCD_OPC_FilterValue, 192, 4, 4, 0, // Skip to: 2133
/* 2129 */    MCD_OPC_Decode, 180, 1, 19, // Opcode: BDZLA
/* 2133 */    MCD_OPC_FilterValue, 128, 6, 4, 0, // Skip to: 2142
/* 2138 */    MCD_OPC_Decode, 161, 1, 19, // Opcode: BDNZLAm
/* 2142 */    MCD_OPC_FilterValue, 160, 6, 4, 0, // Skip to: 2151
/* 2147 */    MCD_OPC_Decode, 162, 1, 19, // Opcode: BDNZLAp
/* 2151 */    MCD_OPC_FilterValue, 192, 6, 4, 0, // Skip to: 2160
/* 2156 */    MCD_OPC_Decode, 181, 1, 19, // Opcode: BDZLAm
/* 2160 */    MCD_OPC_FilterValue, 224, 6, 4, 0, // Skip to: 2169
/* 2165 */    MCD_OPC_Decode, 182, 1, 19, // Opcode: BDZLAp
/* 2169 */    MCD_OPC_Decode, 235, 11, 20, // Opcode: gBCLA
/* 2173 */    MCD_OPC_FilterValue, 17, 10, 0, // Skip to: 2187
/* 2177 */    MCD_OPC_CheckField, 1, 1, 1, 110, 30, // Skip to: 9973
/* 2183 */    MCD_OPC_Decode, 223, 7, 21, // Opcode: SC
/* 2187 */    MCD_OPC_FilterValue, 18, 33, 0, // Skip to: 2224
/* 2191 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 2194 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 2201
/* 2198 */    MCD_OPC_Decode, 120, 22, // Opcode: B
/* 2201 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 2209
/* 2205 */    MCD_OPC_Decode, 194, 1, 22, // Opcode: BL
/* 2209 */    MCD_OPC_FilterValue, 2, 3, 0, // Skip to: 2216
/* 2213 */    MCD_OPC_Decode, 121, 22, // Opcode: BA
/* 2216 */    MCD_OPC_FilterValue, 3, 73, 30, // Skip to: 9973
/* 2220 */    MCD_OPC_Decode, 200, 1, 22, // Opcode: BLA
/* 2224 */    MCD_OPC_FilterValue, 19, 235, 1, // Skip to: 2719
/* 2228 */    MCD_OPC_ExtractField, 0, 11,  // Inst{10-0} ...
/* 2231 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 2251
/* 2235 */    MCD_OPC_CheckField, 21, 2, 0, 52, 30, // Skip to: 9973
/* 2241 */    MCD_OPC_CheckField, 11, 7, 0, 46, 30, // Skip to: 9973
/* 2247 */    MCD_OPC_Decode, 177, 5, 23, // Opcode: MCRF
/* 2251 */    MCD_OPC_FilterValue, 32, 119, 0, // Skip to: 2374
/* 2255 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 2258 */    MCD_OPC_FilterValue, 0, 31, 30, // Skip to: 9973
/* 2262 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 2265 */    MCD_OPC_FilterValue, 128, 4, 10, 0, // Skip to: 2280
/* 2270 */    MCD_OPC_CheckField, 11, 2, 0, 94, 0, // Skip to: 2370
/* 2276 */    MCD_OPC_Decode, 163, 1, 0, // Opcode: BDNZLR
/* 2280 */    MCD_OPC_FilterValue, 192, 4, 10, 0, // Skip to: 2295
/* 2285 */    MCD_OPC_CheckField, 11, 2, 0, 79, 0, // Skip to: 2370
/* 2291 */    MCD_OPC_Decode, 183, 1, 0, // Opcode: BDZLR
/* 2295 */    MCD_OPC_FilterValue, 128, 5, 10, 0, // Skip to: 2310
/* 2300 */    MCD_OPC_CheckField, 11, 2, 0, 64, 0, // Skip to: 2370
/* 2306 */    MCD_OPC_Decode, 203, 1, 0, // Opcode: BLR
/* 2310 */    MCD_OPC_FilterValue, 128, 6, 10, 0, // Skip to: 2325
/* 2315 */    MCD_OPC_CheckField, 11, 2, 0, 49, 0, // Skip to: 2370
/* 2321 */    MCD_OPC_Decode, 168, 1, 0, // Opcode: BDNZLRm
/* 2325 */    MCD_OPC_FilterValue, 160, 6, 10, 0, // Skip to: 2340
/* 2330 */    MCD_OPC_CheckField, 11, 2, 0, 34, 0, // Skip to: 2370
/* 2336 */    MCD_OPC_Decode, 169, 1, 0, // Opcode: BDNZLRp
/* 2340 */    MCD_OPC_FilterValue, 192, 6, 10, 0, // Skip to: 2355
/* 2345 */    MCD_OPC_CheckField, 11, 2, 0, 19, 0, // Skip to: 2370
/* 2351 */    MCD_OPC_Decode, 188, 1, 0, // Opcode: BDZLRm
/* 2355 */    MCD_OPC_FilterValue, 224, 6, 10, 0, // Skip to: 2370
/* 2360 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 2370
/* 2366 */    MCD_OPC_Decode, 189, 1, 0, // Opcode: BDZLRp
/* 2370 */    MCD_OPC_Decode, 236, 11, 24, // Opcode: gBCLR
/* 2374 */    MCD_OPC_FilterValue, 33, 119, 0, // Skip to: 2497
/* 2378 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 2381 */    MCD_OPC_FilterValue, 0, 164, 29, // Skip to: 9973
/* 2385 */    MCD_OPC_ExtractField, 16, 10,  // Inst{25-16} ...
/* 2388 */    MCD_OPC_FilterValue, 128, 4, 10, 0, // Skip to: 2403
/* 2393 */    MCD_OPC_CheckField, 11, 2, 0, 94, 0, // Skip to: 2493
/* 2399 */    MCD_OPC_Decode, 165, 1, 0, // Opcode: BDNZLRL
/* 2403 */    MCD_OPC_FilterValue, 192, 4, 10, 0, // Skip to: 2418
/* 2408 */    MCD_OPC_CheckField, 11, 2, 0, 79, 0, // Skip to: 2493
/* 2414 */    MCD_OPC_Decode, 185, 1, 0, // Opcode: BDZLRL
/* 2418 */    MCD_OPC_FilterValue, 128, 5, 10, 0, // Skip to: 2433
/* 2423 */    MCD_OPC_CheckField, 11, 2, 0, 64, 0, // Skip to: 2493
/* 2429 */    MCD_OPC_Decode, 205, 1, 0, // Opcode: BLRL
/* 2433 */    MCD_OPC_FilterValue, 128, 6, 10, 0, // Skip to: 2448
/* 2438 */    MCD_OPC_CheckField, 11, 2, 0, 49, 0, // Skip to: 2493
/* 2444 */    MCD_OPC_Decode, 166, 1, 0, // Opcode: BDNZLRLm
/* 2448 */    MCD_OPC_FilterValue, 160, 6, 10, 0, // Skip to: 2463
/* 2453 */    MCD_OPC_CheckField, 11, 2, 0, 34, 0, // Skip to: 2493
/* 2459 */    MCD_OPC_Decode, 167, 1, 0, // Opcode: BDNZLRLp
/* 2463 */    MCD_OPC_FilterValue, 192, 6, 10, 0, // Skip to: 2478
/* 2468 */    MCD_OPC_CheckField, 11, 2, 0, 19, 0, // Skip to: 2493
/* 2474 */    MCD_OPC_Decode, 186, 1, 0, // Opcode: BDZLRLm
/* 2478 */    MCD_OPC_FilterValue, 224, 6, 10, 0, // Skip to: 2493
/* 2483 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 2493
/* 2489 */    MCD_OPC_Decode, 187, 1, 0, // Opcode: BDZLRLp
/* 2493 */    MCD_OPC_Decode, 237, 11, 24, // Opcode: gBCLRL
/* 2497 */    MCD_OPC_FilterValue, 36, 10, 0, // Skip to: 2511
/* 2501 */    MCD_OPC_CheckField, 11, 15, 0, 42, 29, // Skip to: 9973
/* 2507 */    MCD_OPC_Decode, 191, 7, 0, // Opcode: RFID
/* 2511 */    MCD_OPC_FilterValue, 66, 4, 0, // Skip to: 2519
/* 2515 */    MCD_OPC_Decode, 238, 1, 25, // Opcode: CRNOR
/* 2519 */    MCD_OPC_FilterValue, 76, 10, 0, // Skip to: 2533
/* 2523 */    MCD_OPC_CheckField, 11, 15, 0, 20, 29, // Skip to: 9973
/* 2529 */    MCD_OPC_Decode, 192, 7, 0, // Opcode: RFMCI
/* 2533 */    MCD_OPC_FilterValue, 78, 10, 0, // Skip to: 2547
/* 2537 */    MCD_OPC_CheckField, 11, 15, 0, 6, 29, // Skip to: 9973
/* 2543 */    MCD_OPC_Decode, 189, 7, 0, // Opcode: RFDI
/* 2547 */    MCD_OPC_FilterValue, 100, 10, 0, // Skip to: 2561
/* 2551 */    MCD_OPC_CheckField, 11, 15, 0, 248, 28, // Skip to: 9973
/* 2557 */    MCD_OPC_Decode, 190, 7, 0, // Opcode: RFI
/* 2561 */    MCD_OPC_FilterValue, 102, 10, 0, // Skip to: 2575
/* 2565 */    MCD_OPC_CheckField, 11, 15, 0, 234, 28, // Skip to: 9973
/* 2571 */    MCD_OPC_Decode, 188, 7, 0, // Opcode: RFCI
/* 2575 */    MCD_OPC_FilterValue, 130, 2, 4, 0, // Skip to: 2584
/* 2580 */    MCD_OPC_Decode, 235, 1, 25, // Opcode: CRANDC
/* 2584 */    MCD_OPC_FilterValue, 172, 2, 10, 0, // Skip to: 2599
/* 2589 */    MCD_OPC_CheckField, 11, 15, 0, 210, 28, // Skip to: 9973
/* 2595 */    MCD_OPC_Decode, 214, 4, 0, // Opcode: ISYNC
/* 2599 */    MCD_OPC_FilterValue, 130, 3, 4, 0, // Skip to: 2608
/* 2604 */    MCD_OPC_Decode, 243, 1, 25, // Opcode: CRXOR
/* 2608 */    MCD_OPC_FilterValue, 194, 3, 4, 0, // Skip to: 2617
/* 2613 */    MCD_OPC_Decode, 237, 1, 25, // Opcode: CRNAND
/* 2617 */    MCD_OPC_FilterValue, 130, 4, 4, 0, // Skip to: 2626
/* 2622 */    MCD_OPC_Decode, 234, 1, 25, // Opcode: CRAND
/* 2626 */    MCD_OPC_FilterValue, 194, 4, 4, 0, // Skip to: 2635
/* 2631 */    MCD_OPC_Decode, 236, 1, 25, // Opcode: CREQV
/* 2635 */    MCD_OPC_FilterValue, 194, 6, 4, 0, // Skip to: 2644
/* 2640 */    MCD_OPC_Decode, 240, 1, 25, // Opcode: CRORC
/* 2644 */    MCD_OPC_FilterValue, 130, 7, 4, 0, // Skip to: 2653
/* 2649 */    MCD_OPC_Decode, 239, 1, 25, // Opcode: CROR
/* 2653 */    MCD_OPC_FilterValue, 160, 8, 28, 0, // Skip to: 2686
/* 2658 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 2661 */    MCD_OPC_FilterValue, 0, 140, 28, // Skip to: 9973
/* 2665 */    MCD_OPC_CheckField, 16, 10, 128, 5, 10, 0, // Skip to: 2682
/* 2672 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 2682
/* 2678 */    MCD_OPC_Decode, 148, 1, 0, // Opcode: BCTR
/* 2682 */    MCD_OPC_Decode, 232, 11, 24, // Opcode: gBCCTR
/* 2686 */    MCD_OPC_FilterValue, 161, 8, 114, 28, // Skip to: 9973
/* 2691 */    MCD_OPC_ExtractField, 13, 3,  // Inst{15-13} ...
/* 2694 */    MCD_OPC_FilterValue, 0, 107, 28, // Skip to: 9973
/* 2698 */    MCD_OPC_CheckField, 16, 10, 128, 5, 10, 0, // Skip to: 2715
/* 2705 */    MCD_OPC_CheckField, 11, 2, 0, 4, 0, // Skip to: 2715
/* 2711 */    MCD_OPC_Decode, 150, 1, 0, // Opcode: BCTRL
/* 2715 */    MCD_OPC_Decode, 233, 11, 24, // Opcode: gBCCTRL
/* 2719 */    MCD_OPC_FilterValue, 20, 19, 0, // Skip to: 2742
/* 2723 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2726 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2734
/* 2730 */    MCD_OPC_Decode, 206, 7, 26, // Opcode: RLWIMI
/* 2734 */    MCD_OPC_FilterValue, 1, 67, 28, // Skip to: 9973
/* 2738 */    MCD_OPC_Decode, 209, 7, 26, // Opcode: RLWIMIo
/* 2742 */    MCD_OPC_FilterValue, 21, 19, 0, // Skip to: 2765
/* 2746 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2749 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2757
/* 2753 */    MCD_OPC_Decode, 210, 7, 27, // Opcode: RLWINM
/* 2757 */    MCD_OPC_FilterValue, 1, 44, 28, // Skip to: 9973
/* 2761 */    MCD_OPC_Decode, 213, 7, 27, // Opcode: RLWINMo
/* 2765 */    MCD_OPC_FilterValue, 23, 19, 0, // Skip to: 2788
/* 2769 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2772 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2780
/* 2776 */    MCD_OPC_Decode, 214, 7, 28, // Opcode: RLWNM
/* 2780 */    MCD_OPC_FilterValue, 1, 21, 28, // Skip to: 9973
/* 2784 */    MCD_OPC_Decode, 217, 7, 28, // Opcode: RLWNMo
/* 2788 */    MCD_OPC_FilterValue, 24, 14, 0, // Skip to: 2806
/* 2792 */    MCD_OPC_CheckField, 0, 26, 0, 4, 0, // Skip to: 2802
/* 2798 */    MCD_OPC_Decode, 251, 5, 0, // Opcode: NOP
/* 2802 */    MCD_OPC_Decode, 137, 6, 29, // Opcode: ORI
/* 2806 */    MCD_OPC_FilterValue, 25, 4, 0, // Skip to: 2814
/* 2810 */    MCD_OPC_Decode, 139, 6, 29, // Opcode: ORIS
/* 2814 */    MCD_OPC_FilterValue, 26, 4, 0, // Skip to: 2822
/* 2818 */    MCD_OPC_Decode, 214, 10, 29, // Opcode: XORI
/* 2822 */    MCD_OPC_FilterValue, 27, 4, 0, // Skip to: 2830
/* 2826 */    MCD_OPC_Decode, 216, 10, 29, // Opcode: XORIS
/* 2830 */    MCD_OPC_FilterValue, 28, 3, 0, // Skip to: 2837
/* 2834 */    MCD_OPC_Decode, 80, 29, // Opcode: ANDIo
/* 2837 */    MCD_OPC_FilterValue, 29, 3, 0, // Skip to: 2844
/* 2841 */    MCD_OPC_Decode, 78, 29, // Opcode: ANDISo
/* 2844 */    MCD_OPC_FilterValue, 30, 134, 0, // Skip to: 2982
/* 2848 */    MCD_OPC_ExtractField, 2, 3,  // Inst{4-2} ...
/* 2851 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 2874
/* 2855 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2858 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2866
/* 2862 */    MCD_OPC_Decode, 198, 7, 30, // Opcode: RLDICL
/* 2866 */    MCD_OPC_FilterValue, 1, 191, 27, // Skip to: 9973
/* 2870 */    MCD_OPC_Decode, 200, 7, 30, // Opcode: RLDICLo
/* 2874 */    MCD_OPC_FilterValue, 1, 19, 0, // Skip to: 2897
/* 2878 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2881 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2889
/* 2885 */    MCD_OPC_Decode, 201, 7, 30, // Opcode: RLDICR
/* 2889 */    MCD_OPC_FilterValue, 1, 168, 27, // Skip to: 9973
/* 2893 */    MCD_OPC_Decode, 202, 7, 30, // Opcode: RLDICRo
/* 2897 */    MCD_OPC_FilterValue, 2, 19, 0, // Skip to: 2920
/* 2901 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2904 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2912
/* 2908 */    MCD_OPC_Decode, 197, 7, 30, // Opcode: RLDIC
/* 2912 */    MCD_OPC_FilterValue, 1, 145, 27, // Skip to: 9973
/* 2916 */    MCD_OPC_Decode, 203, 7, 30, // Opcode: RLDICo
/* 2920 */    MCD_OPC_FilterValue, 3, 19, 0, // Skip to: 2943
/* 2924 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 2927 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2935
/* 2931 */    MCD_OPC_Decode, 204, 7, 31, // Opcode: RLDIMI
/* 2935 */    MCD_OPC_FilterValue, 1, 122, 27, // Skip to: 9973
/* 2939 */    MCD_OPC_Decode, 205, 7, 31, // Opcode: RLDIMIo
/* 2943 */    MCD_OPC_FilterValue, 4, 114, 27, // Skip to: 9973
/* 2947 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 2950 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 2958
/* 2954 */    MCD_OPC_Decode, 193, 7, 32, // Opcode: RLDCL
/* 2958 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 2966
/* 2962 */    MCD_OPC_Decode, 194, 7, 32, // Opcode: RLDCLo
/* 2966 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 2974
/* 2970 */    MCD_OPC_Decode, 195, 7, 32, // Opcode: RLDCR
/* 2974 */    MCD_OPC_FilterValue, 3, 83, 27, // Skip to: 9973
/* 2978 */    MCD_OPC_Decode, 196, 7, 32, // Opcode: RLDCRo
/* 2982 */    MCD_OPC_FilterValue, 31, 179, 12, // Skip to: 6237
/* 2986 */    MCD_OPC_ExtractField, 2, 4,  // Inst{5-2} ...
/* 2989 */    MCD_OPC_FilterValue, 0, 73, 0, // Skip to: 3066
/* 2993 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 2996 */    MCD_OPC_FilterValue, 0, 31, 0, // Skip to: 3031
/* 3000 */    MCD_OPC_ExtractField, 21, 2,  // Inst{22-21} ...
/* 3003 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 3017
/* 3007 */    MCD_OPC_CheckField, 0, 2, 0, 48, 27, // Skip to: 9973
/* 3013 */    MCD_OPC_Decode, 224, 1, 33, // Opcode: CMPW
/* 3017 */    MCD_OPC_FilterValue, 1, 40, 27, // Skip to: 9973
/* 3021 */    MCD_OPC_CheckField, 0, 2, 0, 34, 27, // Skip to: 9973
/* 3027 */    MCD_OPC_Decode, 218, 1, 34, // Opcode: CMPD
/* 3031 */    MCD_OPC_FilterValue, 1, 26, 27, // Skip to: 9973
/* 3035 */    MCD_OPC_ExtractField, 21, 2,  // Inst{22-21} ...
/* 3038 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 3052
/* 3042 */    MCD_OPC_CheckField, 0, 2, 0, 13, 27, // Skip to: 9973
/* 3048 */    MCD_OPC_Decode, 222, 1, 33, // Opcode: CMPLW
/* 3052 */    MCD_OPC_FilterValue, 1, 5, 27, // Skip to: 9973
/* 3056 */    MCD_OPC_CheckField, 0, 2, 0, 255, 26, // Skip to: 9973
/* 3062 */    MCD_OPC_Decode, 220, 1, 34, // Opcode: CMPLD
/* 3066 */    MCD_OPC_FilterValue, 1, 65, 0, // Skip to: 3135
/* 3070 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3073 */    MCD_OPC_FilterValue, 4, 16, 0, // Skip to: 3093
/* 3077 */    MCD_OPC_CheckField, 16, 1, 0, 234, 26, // Skip to: 9973
/* 3083 */    MCD_OPC_CheckField, 1, 1, 1, 228, 26, // Skip to: 9973
/* 3089 */    MCD_OPC_Decode, 209, 10, 35, // Opcode: WRTEE
/* 3093 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 3107
/* 3097 */    MCD_OPC_CheckField, 1, 1, 1, 214, 26, // Skip to: 9973
/* 3103 */    MCD_OPC_Decode, 210, 10, 36, // Opcode: WRTEEI
/* 3107 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 3121
/* 3111 */    MCD_OPC_CheckField, 0, 2, 2, 200, 26, // Skip to: 9973
/* 3117 */    MCD_OPC_Decode, 183, 5, 37, // Opcode: MFDCR
/* 3121 */    MCD_OPC_FilterValue, 14, 192, 26, // Skip to: 9973
/* 3125 */    MCD_OPC_CheckField, 0, 2, 2, 186, 26, // Skip to: 9973
/* 3131 */    MCD_OPC_Decode, 206, 5, 37, // Opcode: MTDCR
/* 3135 */    MCD_OPC_FilterValue, 2, 44, 0, // Skip to: 3183
/* 3139 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3142 */    MCD_OPC_FilterValue, 0, 23, 0, // Skip to: 3169
/* 3146 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3149 */    MCD_OPC_FilterValue, 0, 164, 26, // Skip to: 9973
/* 3153 */    MCD_OPC_CheckField, 11, 15, 128, 248, 1, 4, 0, // Skip to: 3165
/* 3161 */    MCD_OPC_Decode, 134, 9, 0, // Opcode: TRAP
/* 3165 */    MCD_OPC_Decode, 135, 9, 38, // Opcode: TW
/* 3169 */    MCD_OPC_FilterValue, 2, 144, 26, // Skip to: 9973
/* 3173 */    MCD_OPC_CheckField, 0, 2, 0, 138, 26, // Skip to: 9973
/* 3179 */    MCD_OPC_Decode, 246, 8, 39, // Opcode: TD
/* 3183 */    MCD_OPC_FilterValue, 3, 201, 0, // Skip to: 3388
/* 3187 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3190 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 3213
/* 3194 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3197 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 3205
/* 3201 */    MCD_OPC_Decode, 150, 5, 40, // Opcode: LVSL
/* 3205 */    MCD_OPC_FilterValue, 2, 108, 26, // Skip to: 9973
/* 3209 */    MCD_OPC_Decode, 147, 5, 40, // Opcode: LVEBX
/* 3213 */    MCD_OPC_FilterValue, 1, 19, 0, // Skip to: 3236
/* 3217 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3220 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 3228
/* 3224 */    MCD_OPC_Decode, 151, 5, 40, // Opcode: LVSR
/* 3228 */    MCD_OPC_FilterValue, 2, 85, 26, // Skip to: 9973
/* 3232 */    MCD_OPC_Decode, 148, 5, 40, // Opcode: LVEHX
/* 3236 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 3250
/* 3240 */    MCD_OPC_CheckField, 0, 2, 2, 71, 26, // Skip to: 9973
/* 3246 */    MCD_OPC_Decode, 149, 5, 40, // Opcode: LVEWX
/* 3250 */    MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 3264
/* 3254 */    MCD_OPC_CheckField, 0, 2, 2, 57, 26, // Skip to: 9973
/* 3260 */    MCD_OPC_Decode, 152, 5, 40, // Opcode: LVX
/* 3264 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 3278
/* 3268 */    MCD_OPC_CheckField, 0, 2, 2, 43, 26, // Skip to: 9973
/* 3274 */    MCD_OPC_Decode, 188, 8, 40, // Opcode: STVEBX
/* 3278 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 3292
/* 3282 */    MCD_OPC_CheckField, 0, 2, 2, 29, 26, // Skip to: 9973
/* 3288 */    MCD_OPC_Decode, 189, 8, 40, // Opcode: STVEHX
/* 3292 */    MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 3306
/* 3296 */    MCD_OPC_CheckField, 0, 2, 2, 15, 26, // Skip to: 9973
/* 3302 */    MCD_OPC_Decode, 190, 8, 40, // Opcode: STVEWX
/* 3306 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 3320
/* 3310 */    MCD_OPC_CheckField, 0, 2, 2, 1, 26, // Skip to: 9973
/* 3316 */    MCD_OPC_Decode, 191, 8, 40, // Opcode: STVX
/* 3320 */    MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 3334
/* 3324 */    MCD_OPC_CheckField, 0, 2, 2, 243, 25, // Skip to: 9973
/* 3330 */    MCD_OPC_Decode, 153, 5, 40, // Opcode: LVXL
/* 3334 */    MCD_OPC_FilterValue, 14, 16, 0, // Skip to: 3354
/* 3338 */    MCD_OPC_CheckField, 21, 5, 0, 229, 25, // Skip to: 9973
/* 3344 */    MCD_OPC_CheckField, 0, 2, 0, 223, 25, // Skip to: 9973
/* 3350 */    MCD_OPC_Decode, 252, 1, 41, // Opcode: DCCCI
/* 3354 */    MCD_OPC_FilterValue, 15, 10, 0, // Skip to: 3368
/* 3358 */    MCD_OPC_CheckField, 0, 2, 2, 209, 25, // Skip to: 9973
/* 3364 */    MCD_OPC_Decode, 192, 8, 40, // Opcode: STVXL
/* 3368 */    MCD_OPC_FilterValue, 30, 201, 25, // Skip to: 9973
/* 3372 */    MCD_OPC_CheckField, 21, 5, 0, 195, 25, // Skip to: 9973
/* 3378 */    MCD_OPC_CheckField, 0, 2, 0, 189, 25, // Skip to: 9973
/* 3384 */    MCD_OPC_Decode, 205, 4, 41, // Opcode: ICCCI
/* 3388 */    MCD_OPC_FilterValue, 4, 22, 1, // Skip to: 3670
/* 3392 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3395 */    MCD_OPC_FilterValue, 0, 35, 0, // Skip to: 3434
/* 3399 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3402 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 3410
/* 3406 */    MCD_OPC_Decode, 210, 8, 42, // Opcode: SUBFC
/* 3410 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 3418
/* 3414 */    MCD_OPC_Decode, 213, 8, 42, // Opcode: SUBFCo
/* 3418 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3426
/* 3422 */    MCD_OPC_Decode, 227, 5, 43, // Opcode: MULHDU
/* 3426 */    MCD_OPC_FilterValue, 3, 143, 25, // Skip to: 9973
/* 3430 */    MCD_OPC_Decode, 228, 5, 43, // Opcode: MULHDUo
/* 3434 */    MCD_OPC_FilterValue, 1, 19, 0, // Skip to: 3457
/* 3438 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3441 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 3449
/* 3445 */    MCD_OPC_Decode, 207, 8, 42, // Opcode: SUBF
/* 3449 */    MCD_OPC_FilterValue, 1, 120, 25, // Skip to: 9973
/* 3453 */    MCD_OPC_Decode, 228, 8, 42, // Opcode: SUBFo
/* 3457 */    MCD_OPC_FilterValue, 2, 19, 0, // Skip to: 3480
/* 3461 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3464 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3472
/* 3468 */    MCD_OPC_Decode, 226, 5, 43, // Opcode: MULHD
/* 3472 */    MCD_OPC_FilterValue, 3, 97, 25, // Skip to: 9973
/* 3476 */    MCD_OPC_Decode, 229, 5, 43, // Opcode: MULHDo
/* 3480 */    MCD_OPC_FilterValue, 3, 31, 0, // Skip to: 3515
/* 3484 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3487 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 3501
/* 3491 */    MCD_OPC_CheckField, 11, 5, 0, 76, 25, // Skip to: 9973
/* 3497 */    MCD_OPC_Decode, 247, 5, 44, // Opcode: NEG
/* 3501 */    MCD_OPC_FilterValue, 1, 68, 25, // Skip to: 9973
/* 3505 */    MCD_OPC_CheckField, 11, 5, 0, 62, 25, // Skip to: 9973
/* 3511 */    MCD_OPC_Decode, 250, 5, 44, // Opcode: NEGo
/* 3515 */    MCD_OPC_FilterValue, 4, 19, 0, // Skip to: 3538
/* 3519 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3522 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 3530
/* 3526 */    MCD_OPC_Decode, 214, 8, 42, // Opcode: SUBFE
/* 3530 */    MCD_OPC_FilterValue, 1, 39, 25, // Skip to: 9973
/* 3534 */    MCD_OPC_Decode, 217, 8, 42, // Opcode: SUBFEo
/* 3538 */    MCD_OPC_FilterValue, 6, 31, 0, // Skip to: 3573
/* 3542 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3545 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 3559
/* 3549 */    MCD_OPC_CheckField, 11, 5, 0, 18, 25, // Skip to: 9973
/* 3555 */    MCD_OPC_Decode, 224, 8, 44, // Opcode: SUBFZE
/* 3559 */    MCD_OPC_FilterValue, 1, 10, 25, // Skip to: 9973
/* 3563 */    MCD_OPC_CheckField, 11, 5, 0, 4, 25, // Skip to: 9973
/* 3569 */    MCD_OPC_Decode, 227, 8, 44, // Opcode: SUBFZEo
/* 3573 */    MCD_OPC_FilterValue, 7, 47, 0, // Skip to: 3624
/* 3577 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3580 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 3594
/* 3584 */    MCD_OPC_CheckField, 11, 5, 0, 239, 24, // Skip to: 9973
/* 3590 */    MCD_OPC_Decode, 220, 8, 44, // Opcode: SUBFME
/* 3594 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 3608
/* 3598 */    MCD_OPC_CheckField, 11, 5, 0, 225, 24, // Skip to: 9973
/* 3604 */    MCD_OPC_Decode, 223, 8, 44, // Opcode: SUBFMEo
/* 3608 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3616
/* 3612 */    MCD_OPC_Decode, 234, 5, 43, // Opcode: MULLD
/* 3616 */    MCD_OPC_FilterValue, 3, 209, 24, // Skip to: 9973
/* 3620 */    MCD_OPC_Decode, 235, 5, 43, // Opcode: MULLDo
/* 3624 */    MCD_OPC_FilterValue, 14, 19, 0, // Skip to: 3647
/* 3628 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3631 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3639
/* 3635 */    MCD_OPC_Decode, 254, 1, 43, // Opcode: DIVDU
/* 3639 */    MCD_OPC_FilterValue, 3, 186, 24, // Skip to: 9973
/* 3643 */    MCD_OPC_Decode, 255, 1, 43, // Opcode: DIVDUo
/* 3647 */    MCD_OPC_FilterValue, 15, 178, 24, // Skip to: 9973
/* 3651 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3654 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3662
/* 3658 */    MCD_OPC_Decode, 253, 1, 43, // Opcode: DIVD
/* 3662 */    MCD_OPC_FilterValue, 3, 163, 24, // Skip to: 9973
/* 3666 */    MCD_OPC_Decode, 128, 2, 43, // Opcode: DIVDo
/* 3670 */    MCD_OPC_FilterValue, 5, 233, 0, // Skip to: 3907
/* 3674 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3677 */    MCD_OPC_FilterValue, 0, 33, 0, // Skip to: 3714
/* 3681 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3684 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 3691
/* 3688 */    MCD_OPC_Decode, 29, 42, // Opcode: ADDC
/* 3691 */    MCD_OPC_FilterValue, 1, 3, 0, // Skip to: 3698
/* 3695 */    MCD_OPC_Decode, 32, 42, // Opcode: ADDCo
/* 3698 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3706
/* 3702 */    MCD_OPC_Decode, 231, 5, 42, // Opcode: MULHWU
/* 3706 */    MCD_OPC_FilterValue, 3, 119, 24, // Skip to: 9973
/* 3710 */    MCD_OPC_Decode, 232, 5, 42, // Opcode: MULHWUo
/* 3714 */    MCD_OPC_FilterValue, 2, 19, 0, // Skip to: 3737
/* 3718 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3721 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3729
/* 3725 */    MCD_OPC_Decode, 230, 5, 42, // Opcode: MULHW
/* 3729 */    MCD_OPC_FilterValue, 3, 96, 24, // Skip to: 9973
/* 3733 */    MCD_OPC_Decode, 233, 5, 42, // Opcode: MULHWo
/* 3737 */    MCD_OPC_FilterValue, 4, 17, 0, // Skip to: 3758
/* 3741 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3744 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 3751
/* 3748 */    MCD_OPC_Decode, 33, 42, // Opcode: ADDE
/* 3751 */    MCD_OPC_FilterValue, 1, 74, 24, // Skip to: 9973
/* 3755 */    MCD_OPC_Decode, 36, 42, // Opcode: ADDEo
/* 3758 */    MCD_OPC_FilterValue, 6, 29, 0, // Skip to: 3791
/* 3762 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3765 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 3778
/* 3769 */    MCD_OPC_CheckField, 11, 5, 0, 54, 24, // Skip to: 9973
/* 3775 */    MCD_OPC_Decode, 65, 44, // Opcode: ADDZE
/* 3778 */    MCD_OPC_FilterValue, 1, 47, 24, // Skip to: 9973
/* 3782 */    MCD_OPC_CheckField, 11, 5, 0, 41, 24, // Skip to: 9973
/* 3788 */    MCD_OPC_Decode, 68, 44, // Opcode: ADDZEo
/* 3791 */    MCD_OPC_FilterValue, 7, 45, 0, // Skip to: 3840
/* 3795 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3798 */    MCD_OPC_FilterValue, 0, 9, 0, // Skip to: 3811
/* 3802 */    MCD_OPC_CheckField, 11, 5, 0, 21, 24, // Skip to: 9973
/* 3808 */    MCD_OPC_Decode, 61, 44, // Opcode: ADDME
/* 3811 */    MCD_OPC_FilterValue, 1, 9, 0, // Skip to: 3824
/* 3815 */    MCD_OPC_CheckField, 11, 5, 0, 8, 24, // Skip to: 9973
/* 3821 */    MCD_OPC_Decode, 64, 44, // Opcode: ADDMEo
/* 3824 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3832
/* 3828 */    MCD_OPC_Decode, 238, 5, 42, // Opcode: MULLW
/* 3832 */    MCD_OPC_FilterValue, 3, 249, 23, // Skip to: 9973
/* 3836 */    MCD_OPC_Decode, 239, 5, 42, // Opcode: MULLWo
/* 3840 */    MCD_OPC_FilterValue, 8, 17, 0, // Skip to: 3861
/* 3844 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3847 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 3854
/* 3851 */    MCD_OPC_Decode, 22, 42, // Opcode: ADD4
/* 3854 */    MCD_OPC_FilterValue, 1, 227, 23, // Skip to: 9973
/* 3858 */    MCD_OPC_Decode, 24, 42, // Opcode: ADD4o
/* 3861 */    MCD_OPC_FilterValue, 14, 19, 0, // Skip to: 3884
/* 3865 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3868 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3876
/* 3872 */    MCD_OPC_Decode, 130, 2, 42, // Opcode: DIVWU
/* 3876 */    MCD_OPC_FilterValue, 3, 205, 23, // Skip to: 9973
/* 3880 */    MCD_OPC_Decode, 131, 2, 42, // Opcode: DIVWUo
/* 3884 */    MCD_OPC_FilterValue, 15, 197, 23, // Skip to: 9973
/* 3888 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 3891 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 3899
/* 3895 */    MCD_OPC_Decode, 129, 2, 42, // Opcode: DIVW
/* 3899 */    MCD_OPC_FilterValue, 3, 182, 23, // Skip to: 9973
/* 3903 */    MCD_OPC_Decode, 132, 2, 42, // Opcode: DIVWo
/* 3907 */    MCD_OPC_FilterValue, 6, 101, 0, // Skip to: 4012
/* 3911 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 3914 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 3928
/* 3918 */    MCD_OPC_CheckField, 1, 1, 0, 161, 23, // Skip to: 9973
/* 3924 */    MCD_OPC_Decode, 174, 5, 45, // Opcode: LXVDSX
/* 3928 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 3942
/* 3932 */    MCD_OPC_CheckField, 1, 1, 0, 147, 23, // Skip to: 9973
/* 3938 */    MCD_OPC_Decode, 172, 5, 46, // Opcode: LXSDX
/* 3942 */    MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 3956
/* 3946 */    MCD_OPC_CheckField, 1, 1, 0, 133, 23, // Skip to: 9973
/* 3952 */    MCD_OPC_Decode, 204, 8, 46, // Opcode: STXSDX
/* 3956 */    MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 3970
/* 3960 */    MCD_OPC_CheckField, 1, 1, 0, 119, 23, // Skip to: 9973
/* 3966 */    MCD_OPC_Decode, 175, 5, 45, // Opcode: LXVW4X
/* 3970 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 3984
/* 3974 */    MCD_OPC_CheckField, 1, 1, 0, 105, 23, // Skip to: 9973
/* 3980 */    MCD_OPC_Decode, 173, 5, 45, // Opcode: LXVD2X
/* 3984 */    MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 3998
/* 3988 */    MCD_OPC_CheckField, 1, 1, 0, 91, 23, // Skip to: 9973
/* 3994 */    MCD_OPC_Decode, 206, 8, 45, // Opcode: STXVW4X
/* 3998 */    MCD_OPC_FilterValue, 30, 83, 23, // Skip to: 9973
/* 4002 */    MCD_OPC_CheckField, 1, 1, 0, 77, 23, // Skip to: 9973
/* 4008 */    MCD_OPC_Decode, 205, 8, 45, // Opcode: STXVD2X
/* 4012 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 4026
/* 4016 */    MCD_OPC_CheckField, 0, 2, 2, 63, 23, // Skip to: 9973
/* 4022 */    MCD_OPC_Decode, 212, 4, 47, // Opcode: ISEL
/* 4026 */    MCD_OPC_FilterValue, 8, 43, 0, // Skip to: 4073
/* 4030 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4033 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 4053
/* 4037 */    MCD_OPC_CheckField, 6, 6, 4, 42, 23, // Skip to: 9973
/* 4043 */    MCD_OPC_CheckField, 0, 2, 0, 36, 23, // Skip to: 9973
/* 4049 */    MCD_OPC_Decode, 200, 5, 48, // Opcode: MTCRF
/* 4053 */    MCD_OPC_FilterValue, 1, 28, 23, // Skip to: 9973
/* 4057 */    MCD_OPC_CheckField, 6, 6, 4, 22, 23, // Skip to: 9973
/* 4063 */    MCD_OPC_CheckField, 0, 2, 0, 16, 23, // Skip to: 9973
/* 4069 */    MCD_OPC_Decode, 218, 5, 49, // Opcode: MTOCRF
/* 4073 */    MCD_OPC_FilterValue, 9, 246, 1, // Skip to: 4579
/* 4077 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 4080 */    MCD_OPC_FilterValue, 0, 43, 0, // Skip to: 4127
/* 4084 */    MCD_OPC_ExtractField, 20, 1,  // Inst{20} ...
/* 4087 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 4107
/* 4091 */    MCD_OPC_CheckField, 11, 9, 0, 244, 22, // Skip to: 9973
/* 4097 */    MCD_OPC_CheckField, 0, 2, 2, 238, 22, // Skip to: 9973
/* 4103 */    MCD_OPC_Decode, 179, 5, 35, // Opcode: MFCR
/* 4107 */    MCD_OPC_FilterValue, 1, 230, 22, // Skip to: 9973
/* 4111 */    MCD_OPC_CheckField, 11, 1, 0, 224, 22, // Skip to: 9973
/* 4117 */    MCD_OPC_CheckField, 0, 2, 2, 218, 22, // Skip to: 9973
/* 4123 */    MCD_OPC_Decode, 189, 5, 50, // Opcode: MFOCRF
/* 4127 */    MCD_OPC_FilterValue, 2, 16, 0, // Skip to: 4147
/* 4131 */    MCD_OPC_CheckField, 11, 10, 0, 204, 22, // Skip to: 9973
/* 4137 */    MCD_OPC_CheckField, 0, 2, 2, 198, 22, // Skip to: 9973
/* 4143 */    MCD_OPC_Decode, 188, 5, 35, // Opcode: MFMSR
/* 4147 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 4161
/* 4151 */    MCD_OPC_CheckField, 1, 1, 0, 184, 22, // Skip to: 9973
/* 4157 */    MCD_OPC_Decode, 216, 5, 51, // Opcode: MTMSR
/* 4161 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 4175
/* 4165 */    MCD_OPC_CheckField, 1, 1, 0, 170, 22, // Skip to: 9973
/* 4171 */    MCD_OPC_Decode, 217, 5, 51, // Opcode: MTMSRD
/* 4175 */    MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 4189
/* 4179 */    MCD_OPC_CheckField, 1, 1, 0, 156, 22, // Skip to: 9973
/* 4185 */    MCD_OPC_Decode, 221, 5, 52, // Opcode: MTSR
/* 4189 */    MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 4203
/* 4193 */    MCD_OPC_CheckField, 1, 1, 0, 142, 22, // Skip to: 9973
/* 4199 */    MCD_OPC_Decode, 222, 5, 53, // Opcode: MTSRIN
/* 4203 */    MCD_OPC_FilterValue, 8, 16, 0, // Skip to: 4223
/* 4207 */    MCD_OPC_CheckField, 16, 10, 0, 128, 22, // Skip to: 9973
/* 4213 */    MCD_OPC_CheckField, 0, 2, 0, 122, 22, // Skip to: 9973
/* 4219 */    MCD_OPC_Decode, 250, 8, 54, // Opcode: TLBIEL
/* 4223 */    MCD_OPC_FilterValue, 9, 16, 0, // Skip to: 4243
/* 4227 */    MCD_OPC_CheckField, 16, 5, 0, 108, 22, // Skip to: 9973
/* 4233 */    MCD_OPC_CheckField, 0, 2, 0, 102, 22, // Skip to: 9973
/* 4239 */    MCD_OPC_Decode, 249, 8, 53, // Opcode: TLBIE
/* 4243 */    MCD_OPC_FilterValue, 10, 32, 0, // Skip to: 4279
/* 4247 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4250 */    MCD_OPC_FilterValue, 2, 87, 22, // Skip to: 9973
/* 4254 */    MCD_OPC_ExtractField, 11, 10,  // Inst{20-11} ...
/* 4257 */    MCD_OPC_FilterValue, 128, 2, 4, 0, // Skip to: 4266
/* 4262 */    MCD_OPC_Decode, 186, 5, 35, // Opcode: MFLR
/* 4266 */    MCD_OPC_FilterValue, 160, 2, 4, 0, // Skip to: 4275
/* 4271 */    MCD_OPC_Decode, 181, 5, 35, // Opcode: MFCTR
/* 4275 */    MCD_OPC_Decode, 191, 5, 37, // Opcode: MFSPR
/* 4279 */    MCD_OPC_FilterValue, 11, 25, 0, // Skip to: 4308
/* 4283 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4286 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 4300
/* 4290 */    MCD_OPC_CheckField, 11, 15, 0, 45, 22, // Skip to: 9973
/* 4296 */    MCD_OPC_Decode, 248, 8, 0, // Opcode: TLBIA
/* 4300 */    MCD_OPC_FilterValue, 2, 37, 22, // Skip to: 9973
/* 4304 */    MCD_OPC_Decode, 194, 5, 37, // Opcode: MFTB
/* 4308 */    MCD_OPC_FilterValue, 12, 16, 0, // Skip to: 4328
/* 4312 */    MCD_OPC_CheckField, 16, 5, 0, 23, 22, // Skip to: 9973
/* 4318 */    MCD_OPC_CheckField, 0, 2, 0, 17, 22, // Skip to: 9973
/* 4324 */    MCD_OPC_Decode, 247, 7, 53, // Opcode: SLBMTE
/* 4328 */    MCD_OPC_FilterValue, 13, 16, 0, // Skip to: 4348
/* 4332 */    MCD_OPC_CheckField, 16, 10, 0, 3, 22, // Skip to: 9973
/* 4338 */    MCD_OPC_CheckField, 0, 2, 0, 253, 21, // Skip to: 9973
/* 4344 */    MCD_OPC_Decode, 245, 7, 54, // Opcode: SLBIE
/* 4348 */    MCD_OPC_FilterValue, 14, 32, 0, // Skip to: 4384
/* 4352 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4355 */    MCD_OPC_FilterValue, 2, 238, 21, // Skip to: 9973
/* 4359 */    MCD_OPC_ExtractField, 11, 10,  // Inst{20-11} ...
/* 4362 */    MCD_OPC_FilterValue, 128, 2, 4, 0, // Skip to: 4371
/* 4367 */    MCD_OPC_Decode, 214, 5, 35, // Opcode: MTLR
/* 4371 */    MCD_OPC_FilterValue, 160, 2, 4, 0, // Skip to: 4380
/* 4376 */    MCD_OPC_Decode, 202, 5, 35, // Opcode: MTCTR
/* 4380 */    MCD_OPC_Decode, 220, 5, 55, // Opcode: MTSPR
/* 4384 */    MCD_OPC_FilterValue, 15, 16, 0, // Skip to: 4404
/* 4388 */    MCD_OPC_CheckField, 11, 15, 0, 203, 21, // Skip to: 9973
/* 4394 */    MCD_OPC_CheckField, 0, 2, 0, 197, 21, // Skip to: 9973
/* 4400 */    MCD_OPC_Decode, 244, 7, 0, // Opcode: SLBIA
/* 4404 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 4418
/* 4408 */    MCD_OPC_CheckField, 1, 1, 1, 183, 21, // Skip to: 9973
/* 4414 */    MCD_OPC_Decode, 192, 5, 52, // Opcode: MFSR
/* 4418 */    MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 4432
/* 4422 */    MCD_OPC_CheckField, 1, 1, 1, 169, 21, // Skip to: 9973
/* 4428 */    MCD_OPC_Decode, 193, 5, 53, // Opcode: MFSRIN
/* 4432 */    MCD_OPC_FilterValue, 24, 16, 0, // Skip to: 4452
/* 4436 */    MCD_OPC_CheckField, 21, 5, 0, 155, 21, // Skip to: 9973
/* 4442 */    MCD_OPC_CheckField, 0, 2, 0, 149, 21, // Skip to: 9973
/* 4448 */    MCD_OPC_Decode, 251, 8, 41, // Opcode: TLBIVAX
/* 4452 */    MCD_OPC_FilterValue, 28, 43, 0, // Skip to: 4499
/* 4456 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4459 */    MCD_OPC_FilterValue, 0, 14, 0, // Skip to: 4477
/* 4463 */    MCD_OPC_CheckField, 21, 5, 0, 4, 0, // Skip to: 4473
/* 4469 */    MCD_OPC_Decode, 128, 9, 41, // Opcode: TLBSX
/* 4473 */    MCD_OPC_Decode, 129, 9, 42, // Opcode: TLBSX2
/* 4477 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 4485
/* 4481 */    MCD_OPC_Decode, 130, 9, 42, // Opcode: TLBSX2D
/* 4485 */    MCD_OPC_FilterValue, 2, 108, 21, // Skip to: 9973
/* 4489 */    MCD_OPC_CheckField, 16, 5, 0, 102, 21, // Skip to: 9973
/* 4495 */    MCD_OPC_Decode, 246, 7, 53, // Opcode: SLBMFEE
/* 4499 */    MCD_OPC_FilterValue, 29, 21, 0, // Skip to: 4524
/* 4503 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4506 */    MCD_OPC_FilterValue, 0, 87, 21, // Skip to: 9973
/* 4510 */    MCD_OPC_CheckField, 11, 15, 0, 4, 0, // Skip to: 4520
/* 4516 */    MCD_OPC_Decode, 254, 8, 0, // Opcode: TLBRE
/* 4520 */    MCD_OPC_Decode, 255, 8, 56, // Opcode: TLBRE2
/* 4524 */    MCD_OPC_FilterValue, 30, 31, 0, // Skip to: 4559
/* 4528 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4531 */    MCD_OPC_FilterValue, 0, 62, 21, // Skip to: 9973
/* 4535 */    MCD_OPC_CheckField, 11, 15, 0, 4, 0, // Skip to: 4545
/* 4541 */    MCD_OPC_Decode, 132, 9, 0, // Opcode: TLBWE
/* 4545 */    MCD_OPC_CheckField, 16, 10, 0, 4, 0, // Skip to: 4555
/* 4551 */    MCD_OPC_Decode, 252, 8, 54, // Opcode: TLBLD
/* 4555 */    MCD_OPC_Decode, 133, 9, 56, // Opcode: TLBWE2
/* 4559 */    MCD_OPC_FilterValue, 31, 34, 21, // Skip to: 9973
/* 4563 */    MCD_OPC_CheckField, 16, 10, 0, 28, 21, // Skip to: 9973
/* 4569 */    MCD_OPC_CheckField, 0, 2, 0, 22, 21, // Skip to: 9973
/* 4575 */    MCD_OPC_Decode, 253, 8, 54, // Opcode: TLBLI
/* 4579 */    MCD_OPC_FilterValue, 10, 22, 1, // Skip to: 4861
/* 4583 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 4586 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 4609
/* 4590 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4593 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 4601
/* 4597 */    MCD_OPC_Decode, 155, 5, 57, // Opcode: LWARX
/* 4601 */    MCD_OPC_FilterValue, 2, 248, 20, // Skip to: 9973
/* 4605 */    MCD_OPC_Decode, 232, 4, 58, // Opcode: LDX
/* 4609 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 4623
/* 4613 */    MCD_OPC_CheckField, 0, 2, 2, 234, 20, // Skip to: 9973
/* 4619 */    MCD_OPC_Decode, 231, 4, 59, // Opcode: LDUX
/* 4623 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 4637
/* 4627 */    MCD_OPC_CheckField, 0, 2, 0, 220, 20, // Skip to: 9973
/* 4633 */    MCD_OPC_Decode, 227, 4, 58, // Opcode: LDARX
/* 4637 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 4651
/* 4641 */    MCD_OPC_CheckField, 0, 2, 2, 206, 20, // Skip to: 9973
/* 4647 */    MCD_OPC_Decode, 166, 8, 58, // Opcode: STDX
/* 4651 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 4665
/* 4655 */    MCD_OPC_CheckField, 0, 2, 2, 192, 20, // Skip to: 9973
/* 4661 */    MCD_OPC_Decode, 165, 8, 60, // Opcode: STDUX
/* 4665 */    MCD_OPC_FilterValue, 10, 10, 0, // Skip to: 4679
/* 4669 */    MCD_OPC_CheckField, 0, 2, 2, 178, 20, // Skip to: 9973
/* 4675 */    MCD_OPC_Decode, 157, 5, 58, // Opcode: LWAX
/* 4679 */    MCD_OPC_FilterValue, 11, 10, 0, // Skip to: 4693
/* 4683 */    MCD_OPC_CheckField, 0, 2, 2, 164, 20, // Skip to: 9973
/* 4689 */    MCD_OPC_Decode, 156, 5, 59, // Opcode: LWAUX
/* 4693 */    MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 4707
/* 4697 */    MCD_OPC_CheckField, 0, 2, 0, 150, 20, // Skip to: 9973
/* 4703 */    MCD_OPC_Decode, 228, 4, 58, // Opcode: LDBRX
/* 4707 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 4721
/* 4711 */    MCD_OPC_CheckField, 0, 2, 2, 136, 20, // Skip to: 9973
/* 4717 */    MCD_OPC_Decode, 146, 5, 61, // Opcode: LSWI
/* 4721 */    MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 4735
/* 4725 */    MCD_OPC_CheckField, 0, 2, 0, 122, 20, // Skip to: 9973
/* 4731 */    MCD_OPC_Decode, 161, 8, 58, // Opcode: STDBRX
/* 4735 */    MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 4749
/* 4739 */    MCD_OPC_CheckField, 0, 2, 2, 108, 20, // Skip to: 9973
/* 4745 */    MCD_OPC_Decode, 187, 8, 61, // Opcode: STSWI
/* 4749 */    MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 4763
/* 4753 */    MCD_OPC_CheckField, 0, 2, 2, 94, 20, // Skip to: 9973
/* 4759 */    MCD_OPC_Decode, 164, 5, 42, // Opcode: LWZCIX
/* 4763 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 4777
/* 4767 */    MCD_OPC_CheckField, 0, 2, 2, 80, 20, // Skip to: 9973
/* 4773 */    MCD_OPC_Decode, 134, 5, 42, // Opcode: LHZCIX
/* 4777 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 4791
/* 4781 */    MCD_OPC_CheckField, 0, 2, 2, 66, 20, // Skip to: 9973
/* 4787 */    MCD_OPC_Decode, 219, 4, 42, // Opcode: LBZCIX
/* 4791 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 4805
/* 4795 */    MCD_OPC_CheckField, 0, 2, 2, 52, 20, // Skip to: 9973
/* 4801 */    MCD_OPC_Decode, 229, 4, 42, // Opcode: LDCIX
/* 4805 */    MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 4819
/* 4809 */    MCD_OPC_CheckField, 0, 2, 2, 38, 20, // Skip to: 9973
/* 4815 */    MCD_OPC_Decode, 196, 8, 42, // Opcode: STWCIX
/* 4819 */    MCD_OPC_FilterValue, 29, 10, 0, // Skip to: 4833
/* 4823 */    MCD_OPC_CheckField, 0, 2, 2, 24, 20, // Skip to: 9973
/* 4829 */    MCD_OPC_Decode, 179, 8, 42, // Opcode: STHCIX
/* 4833 */    MCD_OPC_FilterValue, 30, 10, 0, // Skip to: 4847
/* 4837 */    MCD_OPC_CheckField, 0, 2, 2, 10, 20, // Skip to: 9973
/* 4843 */    MCD_OPC_Decode, 153, 8, 42, // Opcode: STBCIX
/* 4847 */    MCD_OPC_FilterValue, 31, 2, 20, // Skip to: 9973
/* 4851 */    MCD_OPC_CheckField, 0, 2, 2, 252, 19, // Skip to: 9973
/* 4857 */    MCD_OPC_Decode, 162, 8, 42, // Opcode: STDCIX
/* 4861 */    MCD_OPC_FilterValue, 11, 227, 2, // Skip to: 5604
/* 4865 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 4868 */    MCD_OPC_FilterValue, 0, 25, 0, // Skip to: 4897
/* 4872 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4875 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 4889
/* 4879 */    MCD_OPC_CheckField, 25, 1, 0, 224, 19, // Skip to: 9973
/* 4885 */    MCD_OPC_Decode, 204, 4, 62, // Opcode: ICBT
/* 4889 */    MCD_OPC_FilterValue, 2, 216, 19, // Skip to: 9973
/* 4893 */    MCD_OPC_Decode, 169, 5, 57, // Opcode: LWZX
/* 4897 */    MCD_OPC_FilterValue, 1, 25, 0, // Skip to: 4926
/* 4901 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4904 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 4918
/* 4908 */    MCD_OPC_CheckField, 21, 5, 0, 195, 19, // Skip to: 9973
/* 4914 */    MCD_OPC_Decode, 247, 1, 63, // Opcode: DCBST
/* 4918 */    MCD_OPC_FilterValue, 2, 187, 19, // Skip to: 9973
/* 4922 */    MCD_OPC_Decode, 167, 5, 64, // Opcode: LWZUX
/* 4926 */    MCD_OPC_FilterValue, 2, 25, 0, // Skip to: 4955
/* 4930 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4933 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 4947
/* 4937 */    MCD_OPC_CheckField, 21, 5, 0, 166, 19, // Skip to: 9973
/* 4943 */    MCD_OPC_Decode, 245, 1, 63, // Opcode: DCBF
/* 4947 */    MCD_OPC_FilterValue, 2, 158, 19, // Skip to: 9973
/* 4951 */    MCD_OPC_Decode, 224, 4, 57, // Opcode: LBZX
/* 4955 */    MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 4969
/* 4959 */    MCD_OPC_CheckField, 0, 2, 2, 144, 19, // Skip to: 9973
/* 4965 */    MCD_OPC_Decode, 222, 4, 64, // Opcode: LBZUX
/* 4969 */    MCD_OPC_FilterValue, 4, 19, 0, // Skip to: 4992
/* 4973 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 4976 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 4984
/* 4980 */    MCD_OPC_Decode, 197, 8, 57, // Opcode: STWCX
/* 4984 */    MCD_OPC_FilterValue, 2, 121, 19, // Skip to: 9973
/* 4988 */    MCD_OPC_Decode, 202, 8, 57, // Opcode: STWX
/* 4992 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 5006
/* 4996 */    MCD_OPC_CheckField, 0, 2, 2, 107, 19, // Skip to: 9973
/* 5002 */    MCD_OPC_Decode, 200, 8, 65, // Opcode: STWUX
/* 5006 */    MCD_OPC_FilterValue, 6, 19, 0, // Skip to: 5029
/* 5010 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5013 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 5021
/* 5017 */    MCD_OPC_Decode, 163, 8, 58, // Opcode: STDCX
/* 5021 */    MCD_OPC_FilterValue, 2, 84, 19, // Skip to: 9973
/* 5025 */    MCD_OPC_Decode, 158, 8, 57, // Opcode: STBX
/* 5029 */    MCD_OPC_FilterValue, 7, 25, 0, // Skip to: 5058
/* 5033 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5036 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5050
/* 5040 */    MCD_OPC_CheckField, 21, 5, 0, 63, 19, // Skip to: 9973
/* 5046 */    MCD_OPC_Decode, 249, 1, 63, // Opcode: DCBTST
/* 5050 */    MCD_OPC_FilterValue, 2, 55, 19, // Skip to: 9973
/* 5054 */    MCD_OPC_Decode, 156, 8, 65, // Opcode: STBUX
/* 5058 */    MCD_OPC_FilterValue, 8, 25, 0, // Skip to: 5087
/* 5062 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5065 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5079
/* 5069 */    MCD_OPC_CheckField, 21, 5, 0, 34, 19, // Skip to: 9973
/* 5075 */    MCD_OPC_Decode, 248, 1, 63, // Opcode: DCBT
/* 5079 */    MCD_OPC_FilterValue, 2, 26, 19, // Skip to: 9973
/* 5083 */    MCD_OPC_Decode, 139, 5, 57, // Opcode: LHZX
/* 5087 */    MCD_OPC_FilterValue, 9, 10, 0, // Skip to: 5101
/* 5091 */    MCD_OPC_CheckField, 0, 2, 2, 12, 19, // Skip to: 9973
/* 5097 */    MCD_OPC_Decode, 137, 5, 64, // Opcode: LHZUX
/* 5101 */    MCD_OPC_FilterValue, 10, 34, 0, // Skip to: 5139
/* 5105 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5108 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 5131
/* 5112 */    MCD_OPC_ExtractField, 23, 3,  // Inst{25-23} ...
/* 5115 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5123
/* 5119 */    MCD_OPC_Decode, 135, 2, 66, // Opcode: DST
/* 5123 */    MCD_OPC_FilterValue, 4, 238, 18, // Skip to: 9973
/* 5127 */    MCD_OPC_Decode, 141, 2, 66, // Opcode: DSTT
/* 5131 */    MCD_OPC_FilterValue, 2, 230, 18, // Skip to: 9973
/* 5135 */    MCD_OPC_Decode, 128, 5, 57, // Opcode: LHAX
/* 5139 */    MCD_OPC_FilterValue, 11, 34, 0, // Skip to: 5177
/* 5143 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5146 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 5169
/* 5150 */    MCD_OPC_ExtractField, 23, 3,  // Inst{25-23} ...
/* 5153 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5161
/* 5157 */    MCD_OPC_Decode, 137, 2, 66, // Opcode: DSTST
/* 5161 */    MCD_OPC_FilterValue, 4, 200, 18, // Skip to: 9973
/* 5165 */    MCD_OPC_Decode, 139, 2, 66, // Opcode: DSTSTT
/* 5169 */    MCD_OPC_FilterValue, 2, 192, 18, // Skip to: 9973
/* 5173 */    MCD_OPC_Decode, 254, 4, 64, // Opcode: LHAUX
/* 5177 */    MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 5191
/* 5181 */    MCD_OPC_CheckField, 0, 2, 2, 178, 18, // Skip to: 9973
/* 5187 */    MCD_OPC_Decode, 184, 8, 57, // Opcode: STHX
/* 5191 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 5205
/* 5195 */    MCD_OPC_CheckField, 0, 2, 2, 164, 18, // Skip to: 9973
/* 5201 */    MCD_OPC_Decode, 182, 8, 65, // Opcode: STHUX
/* 5205 */    MCD_OPC_FilterValue, 14, 16, 0, // Skip to: 5225
/* 5209 */    MCD_OPC_CheckField, 21, 5, 0, 150, 18, // Skip to: 9973
/* 5215 */    MCD_OPC_CheckField, 0, 2, 0, 144, 18, // Skip to: 9973
/* 5221 */    MCD_OPC_Decode, 246, 1, 63, // Opcode: DCBI
/* 5225 */    MCD_OPC_FilterValue, 16, 19, 0, // Skip to: 5248
/* 5229 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5232 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5240
/* 5236 */    MCD_OPC_Decode, 160, 5, 57, // Opcode: LWBRX
/* 5240 */    MCD_OPC_FilterValue, 2, 121, 18, // Skip to: 9973
/* 5244 */    MCD_OPC_Decode, 249, 4, 67, // Opcode: LFSX
/* 5248 */    MCD_OPC_FilterValue, 17, 25, 0, // Skip to: 5277
/* 5252 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5255 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5269
/* 5259 */    MCD_OPC_CheckField, 11, 15, 0, 100, 18, // Skip to: 9973
/* 5265 */    MCD_OPC_Decode, 131, 9, 0, // Opcode: TLBSYNC
/* 5269 */    MCD_OPC_FilterValue, 2, 92, 18, // Skip to: 9973
/* 5273 */    MCD_OPC_Decode, 248, 4, 68, // Opcode: LFSUX
/* 5277 */    MCD_OPC_FilterValue, 18, 31, 0, // Skip to: 5312
/* 5281 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5284 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 5304
/* 5288 */    MCD_OPC_CheckField, 23, 3, 0, 71, 18, // Skip to: 9973
/* 5294 */    MCD_OPC_CheckField, 11, 10, 0, 65, 18, // Skip to: 9973
/* 5300 */    MCD_OPC_Decode, 233, 8, 69, // Opcode: SYNC
/* 5304 */    MCD_OPC_FilterValue, 2, 57, 18, // Skip to: 9973
/* 5308 */    MCD_OPC_Decode, 243, 4, 70, // Opcode: LFDX
/* 5312 */    MCD_OPC_FilterValue, 19, 10, 0, // Skip to: 5326
/* 5316 */    MCD_OPC_CheckField, 0, 2, 2, 43, 18, // Skip to: 9973
/* 5322 */    MCD_OPC_Decode, 242, 4, 71, // Opcode: LFDUX
/* 5326 */    MCD_OPC_FilterValue, 20, 19, 0, // Skip to: 5349
/* 5330 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5333 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5341
/* 5337 */    MCD_OPC_Decode, 195, 8, 57, // Opcode: STWBRX
/* 5341 */    MCD_OPC_FilterValue, 2, 20, 18, // Skip to: 9973
/* 5345 */    MCD_OPC_Decode, 175, 8, 67, // Opcode: STFSX
/* 5349 */    MCD_OPC_FilterValue, 21, 10, 0, // Skip to: 5363
/* 5353 */    MCD_OPC_CheckField, 0, 2, 2, 6, 18, // Skip to: 9973
/* 5359 */    MCD_OPC_Decode, 174, 8, 72, // Opcode: STFSUX
/* 5363 */    MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 5377
/* 5367 */    MCD_OPC_CheckField, 0, 2, 2, 248, 17, // Skip to: 9973
/* 5373 */    MCD_OPC_Decode, 170, 8, 70, // Opcode: STFDX
/* 5377 */    MCD_OPC_FilterValue, 23, 25, 0, // Skip to: 5406
/* 5381 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5384 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5398
/* 5388 */    MCD_OPC_CheckField, 21, 5, 0, 227, 17, // Skip to: 9973
/* 5394 */    MCD_OPC_Decode, 244, 1, 63, // Opcode: DCBA
/* 5398 */    MCD_OPC_FilterValue, 2, 219, 17, // Skip to: 9973
/* 5402 */    MCD_OPC_Decode, 169, 8, 73, // Opcode: STFDUX
/* 5406 */    MCD_OPC_FilterValue, 24, 10, 0, // Skip to: 5420
/* 5410 */    MCD_OPC_CheckField, 0, 2, 0, 205, 17, // Skip to: 9973
/* 5416 */    MCD_OPC_Decode, 130, 5, 57, // Opcode: LHBRX
/* 5420 */    MCD_OPC_FilterValue, 25, 43, 0, // Skip to: 5467
/* 5424 */    MCD_OPC_ExtractField, 23, 3,  // Inst{25-23} ...
/* 5427 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 5447
/* 5431 */    MCD_OPC_CheckField, 11, 10, 0, 184, 17, // Skip to: 9973
/* 5437 */    MCD_OPC_CheckField, 0, 2, 0, 178, 17, // Skip to: 9973
/* 5443 */    MCD_OPC_Decode, 133, 2, 74, // Opcode: DSS
/* 5447 */    MCD_OPC_FilterValue, 4, 170, 17, // Skip to: 9973
/* 5451 */    MCD_OPC_CheckField, 11, 12, 0, 164, 17, // Skip to: 9973
/* 5457 */    MCD_OPC_CheckField, 0, 2, 0, 158, 17, // Skip to: 9973
/* 5463 */    MCD_OPC_Decode, 134, 2, 0, // Opcode: DSSALL
/* 5467 */    MCD_OPC_FilterValue, 26, 41, 0, // Skip to: 5512
/* 5471 */    MCD_OPC_ExtractField, 1, 1,  // Inst{1} ...
/* 5474 */    MCD_OPC_FilterValue, 0, 20, 0, // Skip to: 5498
/* 5478 */    MCD_OPC_CheckField, 11, 15, 0, 10, 0, // Skip to: 5494
/* 5484 */    MCD_OPC_CheckField, 0, 1, 0, 4, 0, // Skip to: 5494
/* 5490 */    MCD_OPC_Decode, 219, 3, 0, // Opcode: EnforceIEIO
/* 5494 */    MCD_OPC_Decode, 176, 5, 75, // Opcode: MBAR
/* 5498 */    MCD_OPC_FilterValue, 1, 119, 17, // Skip to: 9973
/* 5502 */    MCD_OPC_CheckField, 0, 1, 0, 113, 17, // Skip to: 9973
/* 5508 */    MCD_OPC_Decode, 244, 4, 70, // Opcode: LFIWAX
/* 5512 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 5526
/* 5516 */    MCD_OPC_CheckField, 0, 2, 2, 99, 17, // Skip to: 9973
/* 5522 */    MCD_OPC_Decode, 245, 4, 70, // Opcode: LFIWZX
/* 5526 */    MCD_OPC_FilterValue, 28, 10, 0, // Skip to: 5540
/* 5530 */    MCD_OPC_CheckField, 0, 2, 0, 85, 17, // Skip to: 9973
/* 5536 */    MCD_OPC_Decode, 178, 8, 57, // Opcode: STHBRX
/* 5540 */    MCD_OPC_FilterValue, 30, 25, 0, // Skip to: 5569
/* 5544 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5547 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5561
/* 5551 */    MCD_OPC_CheckField, 21, 5, 0, 64, 17, // Skip to: 9973
/* 5557 */    MCD_OPC_Decode, 203, 4, 63, // Opcode: ICBI
/* 5561 */    MCD_OPC_FilterValue, 2, 56, 17, // Skip to: 9973
/* 5565 */    MCD_OPC_Decode, 171, 8, 70, // Opcode: STFIWX
/* 5569 */    MCD_OPC_FilterValue, 31, 48, 17, // Skip to: 9973
/* 5573 */    MCD_OPC_ExtractField, 21, 5,  // Inst{25-21} ...
/* 5576 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5590
/* 5580 */    MCD_OPC_CheckField, 0, 2, 0, 35, 17, // Skip to: 9973
/* 5586 */    MCD_OPC_Decode, 250, 1, 63, // Opcode: DCBZ
/* 5590 */    MCD_OPC_FilterValue, 1, 27, 17, // Skip to: 9973
/* 5594 */    MCD_OPC_CheckField, 0, 2, 0, 21, 17, // Skip to: 9973
/* 5600 */    MCD_OPC_Decode, 251, 1, 63, // Opcode: DCBZL
/* 5604 */    MCD_OPC_FilterValue, 12, 95, 0, // Skip to: 5703
/* 5608 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 5611 */    MCD_OPC_FilterValue, 0, 19, 0, // Skip to: 5634
/* 5615 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5618 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5626
/* 5622 */    MCD_OPC_Decode, 252, 7, 76, // Opcode: SLW
/* 5626 */    MCD_OPC_FilterValue, 1, 247, 16, // Skip to: 9973
/* 5630 */    MCD_OPC_Decode, 129, 8, 76, // Opcode: SLWo
/* 5634 */    MCD_OPC_FilterValue, 16, 19, 0, // Skip to: 5657
/* 5638 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5641 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5649
/* 5645 */    MCD_OPC_Decode, 145, 8, 76, // Opcode: SRW
/* 5649 */    MCD_OPC_FilterValue, 1, 224, 16, // Skip to: 9973
/* 5653 */    MCD_OPC_Decode, 150, 8, 76, // Opcode: SRWo
/* 5657 */    MCD_OPC_FilterValue, 24, 19, 0, // Skip to: 5680
/* 5661 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5664 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5672
/* 5668 */    MCD_OPC_Decode, 137, 8, 76, // Opcode: SRAW
/* 5672 */    MCD_OPC_FilterValue, 1, 201, 16, // Skip to: 9973
/* 5676 */    MCD_OPC_Decode, 140, 8, 76, // Opcode: SRAWo
/* 5680 */    MCD_OPC_FilterValue, 25, 193, 16, // Skip to: 9973
/* 5684 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5687 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5695
/* 5691 */    MCD_OPC_Decode, 138, 8, 77, // Opcode: SRAWI
/* 5695 */    MCD_OPC_FilterValue, 1, 178, 16, // Skip to: 9973
/* 5699 */    MCD_OPC_Decode, 139, 8, 77, // Opcode: SRAWIo
/* 5703 */    MCD_OPC_FilterValue, 13, 47, 1, // Skip to: 6010
/* 5707 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 5710 */    MCD_OPC_FilterValue, 0, 47, 0, // Skip to: 5761
/* 5714 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5717 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5731
/* 5721 */    MCD_OPC_CheckField, 11, 5, 0, 150, 16, // Skip to: 9973
/* 5727 */    MCD_OPC_Decode, 228, 1, 78, // Opcode: CNTLZW
/* 5731 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 5745
/* 5735 */    MCD_OPC_CheckField, 11, 5, 0, 136, 16, // Skip to: 9973
/* 5741 */    MCD_OPC_Decode, 231, 1, 78, // Opcode: CNTLZWo
/* 5745 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 5753
/* 5749 */    MCD_OPC_Decode, 248, 7, 79, // Opcode: SLD
/* 5753 */    MCD_OPC_FilterValue, 3, 120, 16, // Skip to: 9973
/* 5757 */    MCD_OPC_Decode, 251, 7, 79, // Opcode: SLDo
/* 5761 */    MCD_OPC_FilterValue, 1, 31, 0, // Skip to: 5796
/* 5765 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5768 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5782
/* 5772 */    MCD_OPC_CheckField, 11, 5, 0, 99, 16, // Skip to: 9973
/* 5778 */    MCD_OPC_Decode, 226, 1, 80, // Opcode: CNTLZD
/* 5782 */    MCD_OPC_FilterValue, 1, 91, 16, // Skip to: 9973
/* 5786 */    MCD_OPC_CheckField, 11, 5, 0, 85, 16, // Skip to: 9973
/* 5792 */    MCD_OPC_Decode, 227, 1, 80, // Opcode: CNTLZDo
/* 5796 */    MCD_OPC_FilterValue, 11, 16, 0, // Skip to: 5816
/* 5800 */    MCD_OPC_CheckField, 11, 5, 0, 71, 16, // Skip to: 9973
/* 5806 */    MCD_OPC_CheckField, 0, 2, 0, 65, 16, // Skip to: 9973
/* 5812 */    MCD_OPC_Decode, 143, 6, 78, // Opcode: POPCNTW
/* 5816 */    MCD_OPC_FilterValue, 15, 16, 0, // Skip to: 5836
/* 5820 */    MCD_OPC_CheckField, 11, 5, 0, 51, 16, // Skip to: 9973
/* 5826 */    MCD_OPC_CheckField, 0, 2, 0, 45, 16, // Skip to: 9973
/* 5832 */    MCD_OPC_Decode, 142, 6, 80, // Opcode: POPCNTD
/* 5836 */    MCD_OPC_FilterValue, 16, 19, 0, // Skip to: 5859
/* 5840 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5843 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 5851
/* 5847 */    MCD_OPC_Decode, 141, 8, 79, // Opcode: SRD
/* 5851 */    MCD_OPC_FilterValue, 3, 22, 16, // Skip to: 9973
/* 5855 */    MCD_OPC_Decode, 144, 8, 79, // Opcode: SRDo
/* 5859 */    MCD_OPC_FilterValue, 24, 19, 0, // Skip to: 5882
/* 5863 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5866 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5874
/* 5870 */    MCD_OPC_Decode, 133, 8, 79, // Opcode: SRAD
/* 5874 */    MCD_OPC_FilterValue, 1, 255, 15, // Skip to: 9973
/* 5878 */    MCD_OPC_Decode, 136, 8, 79, // Opcode: SRADo
/* 5882 */    MCD_OPC_FilterValue, 25, 19, 0, // Skip to: 5905
/* 5886 */    MCD_OPC_ExtractField, 0, 1,  // Inst{0} ...
/* 5889 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 5897
/* 5893 */    MCD_OPC_Decode, 134, 8, 81, // Opcode: SRADI
/* 5897 */    MCD_OPC_FilterValue, 1, 232, 15, // Skip to: 9973
/* 5901 */    MCD_OPC_Decode, 135, 8, 81, // Opcode: SRADIo
/* 5905 */    MCD_OPC_FilterValue, 28, 31, 0, // Skip to: 5940
/* 5909 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5912 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5926
/* 5916 */    MCD_OPC_CheckField, 11, 5, 0, 211, 15, // Skip to: 9973
/* 5922 */    MCD_OPC_Decode, 210, 3, 78, // Opcode: EXTSH
/* 5926 */    MCD_OPC_FilterValue, 1, 203, 15, // Skip to: 9973
/* 5930 */    MCD_OPC_CheckField, 11, 5, 0, 197, 15, // Skip to: 9973
/* 5936 */    MCD_OPC_Decode, 214, 3, 78, // Opcode: EXTSHo
/* 5940 */    MCD_OPC_FilterValue, 29, 31, 0, // Skip to: 5975
/* 5944 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5947 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5961
/* 5951 */    MCD_OPC_CheckField, 11, 5, 0, 176, 15, // Skip to: 9973
/* 5957 */    MCD_OPC_Decode, 205, 3, 78, // Opcode: EXTSB
/* 5961 */    MCD_OPC_FilterValue, 1, 168, 15, // Skip to: 9973
/* 5965 */    MCD_OPC_CheckField, 11, 5, 0, 162, 15, // Skip to: 9973
/* 5971 */    MCD_OPC_Decode, 209, 3, 78, // Opcode: EXTSBo
/* 5975 */    MCD_OPC_FilterValue, 30, 154, 15, // Skip to: 9973
/* 5979 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 5982 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 5996
/* 5986 */    MCD_OPC_CheckField, 11, 5, 0, 141, 15, // Skip to: 9973
/* 5992 */    MCD_OPC_Decode, 215, 3, 80, // Opcode: EXTSW
/* 5996 */    MCD_OPC_FilterValue, 1, 133, 15, // Skip to: 9973
/* 6000 */    MCD_OPC_CheckField, 11, 5, 0, 127, 15, // Skip to: 9973
/* 6006 */    MCD_OPC_Decode, 218, 3, 80, // Opcode: EXTSWo
/* 6010 */    MCD_OPC_FilterValue, 14, 197, 0, // Skip to: 6211
/* 6014 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 6017 */    MCD_OPC_FilterValue, 0, 17, 0, // Skip to: 6038
/* 6021 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6024 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 6031
/* 6028 */    MCD_OPC_Decode, 71, 76, // Opcode: AND
/* 6031 */    MCD_OPC_FilterValue, 1, 98, 15, // Skip to: 9973
/* 6035 */    MCD_OPC_Decode, 86, 76, // Opcode: ANDo
/* 6038 */    MCD_OPC_FilterValue, 1, 17, 0, // Skip to: 6059
/* 6042 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6045 */    MCD_OPC_FilterValue, 0, 3, 0, // Skip to: 6052
/* 6049 */    MCD_OPC_Decode, 74, 76, // Opcode: ANDC
/* 6052 */    MCD_OPC_FilterValue, 1, 77, 15, // Skip to: 9973
/* 6056 */    MCD_OPC_Decode, 77, 76, // Opcode: ANDCo
/* 6059 */    MCD_OPC_FilterValue, 3, 19, 0, // Skip to: 6082
/* 6063 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6066 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6074
/* 6070 */    MCD_OPC_Decode, 254, 5, 76, // Opcode: NOR
/* 6074 */    MCD_OPC_FilterValue, 1, 55, 15, // Skip to: 9973
/* 6078 */    MCD_OPC_Decode, 129, 6, 76, // Opcode: NORo
/* 6082 */    MCD_OPC_FilterValue, 8, 19, 0, // Skip to: 6105
/* 6086 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6089 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6097
/* 6093 */    MCD_OPC_Decode, 150, 2, 76, // Opcode: EQV
/* 6097 */    MCD_OPC_FilterValue, 1, 32, 15, // Skip to: 9973
/* 6101 */    MCD_OPC_Decode, 153, 2, 76, // Opcode: EQVo
/* 6105 */    MCD_OPC_FilterValue, 9, 19, 0, // Skip to: 6128
/* 6109 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6112 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6120
/* 6116 */    MCD_OPC_Decode, 211, 10, 76, // Opcode: XOR
/* 6120 */    MCD_OPC_FilterValue, 1, 9, 15, // Skip to: 9973
/* 6124 */    MCD_OPC_Decode, 218, 10, 76, // Opcode: XORo
/* 6128 */    MCD_OPC_FilterValue, 12, 19, 0, // Skip to: 6151
/* 6132 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6135 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6143
/* 6139 */    MCD_OPC_Decode, 133, 6, 76, // Opcode: ORC
/* 6143 */    MCD_OPC_FilterValue, 1, 242, 14, // Skip to: 9973
/* 6147 */    MCD_OPC_Decode, 136, 6, 76, // Opcode: ORCo
/* 6151 */    MCD_OPC_FilterValue, 13, 19, 0, // Skip to: 6174
/* 6155 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6158 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6166
/* 6162 */    MCD_OPC_Decode, 130, 6, 76, // Opcode: OR
/* 6166 */    MCD_OPC_FilterValue, 1, 219, 14, // Skip to: 9973
/* 6170 */    MCD_OPC_Decode, 141, 6, 76, // Opcode: ORo
/* 6174 */    MCD_OPC_FilterValue, 14, 19, 0, // Skip to: 6197
/* 6178 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6181 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6189
/* 6185 */    MCD_OPC_Decode, 243, 5, 76, // Opcode: NAND
/* 6189 */    MCD_OPC_FilterValue, 1, 196, 14, // Skip to: 9973
/* 6193 */    MCD_OPC_Decode, 246, 5, 76, // Opcode: NANDo
/* 6197 */    MCD_OPC_FilterValue, 15, 188, 14, // Skip to: 9973
/* 6201 */    MCD_OPC_CheckField, 0, 2, 0, 182, 14, // Skip to: 9973
/* 6207 */    MCD_OPC_Decode, 216, 1, 76, // Opcode: CMPB
/* 6211 */    MCD_OPC_FilterValue, 15, 174, 14, // Skip to: 9973
/* 6215 */    MCD_OPC_CheckField, 23, 3, 0, 168, 14, // Skip to: 9973
/* 6221 */    MCD_OPC_CheckField, 6, 15, 1, 162, 14, // Skip to: 9973
/* 6227 */    MCD_OPC_CheckField, 0, 2, 0, 156, 14, // Skip to: 9973
/* 6233 */    MCD_OPC_Decode, 208, 10, 69, // Opcode: WAIT
/* 6237 */    MCD_OPC_FilterValue, 32, 4, 0, // Skip to: 6245
/* 6241 */    MCD_OPC_Decode, 162, 5, 82, // Opcode: LWZ
/* 6245 */    MCD_OPC_FilterValue, 33, 4, 0, // Skip to: 6253
/* 6249 */    MCD_OPC_Decode, 165, 5, 82, // Opcode: LWZU
/* 6253 */    MCD_OPC_FilterValue, 34, 4, 0, // Skip to: 6261
/* 6257 */    MCD_OPC_Decode, 217, 4, 82, // Opcode: LBZ
/* 6261 */    MCD_OPC_FilterValue, 35, 4, 0, // Skip to: 6269
/* 6265 */    MCD_OPC_Decode, 220, 4, 82, // Opcode: LBZU
/* 6269 */    MCD_OPC_FilterValue, 36, 4, 0, // Skip to: 6277
/* 6273 */    MCD_OPC_Decode, 193, 8, 82, // Opcode: STW
/* 6277 */    MCD_OPC_FilterValue, 37, 4, 0, // Skip to: 6285
/* 6281 */    MCD_OPC_Decode, 198, 8, 82, // Opcode: STWU
/* 6285 */    MCD_OPC_FilterValue, 38, 4, 0, // Skip to: 6293
/* 6289 */    MCD_OPC_Decode, 151, 8, 82, // Opcode: STB
/* 6293 */    MCD_OPC_FilterValue, 39, 4, 0, // Skip to: 6301
/* 6297 */    MCD_OPC_Decode, 154, 8, 82, // Opcode: STBU
/* 6301 */    MCD_OPC_FilterValue, 40, 4, 0, // Skip to: 6309
/* 6305 */    MCD_OPC_Decode, 132, 5, 82, // Opcode: LHZ
/* 6309 */    MCD_OPC_FilterValue, 41, 4, 0, // Skip to: 6317
/* 6313 */    MCD_OPC_Decode, 135, 5, 82, // Opcode: LHZU
/* 6317 */    MCD_OPC_FilterValue, 42, 4, 0, // Skip to: 6325
/* 6321 */    MCD_OPC_Decode, 250, 4, 82, // Opcode: LHA
/* 6325 */    MCD_OPC_FilterValue, 43, 4, 0, // Skip to: 6333
/* 6329 */    MCD_OPC_Decode, 252, 4, 82, // Opcode: LHAU
/* 6333 */    MCD_OPC_FilterValue, 44, 4, 0, // Skip to: 6341
/* 6337 */    MCD_OPC_Decode, 176, 8, 82, // Opcode: STH
/* 6341 */    MCD_OPC_FilterValue, 45, 4, 0, // Skip to: 6349
/* 6345 */    MCD_OPC_Decode, 180, 8, 82, // Opcode: STHU
/* 6349 */    MCD_OPC_FilterValue, 46, 4, 0, // Skip to: 6357
/* 6353 */    MCD_OPC_Decode, 145, 5, 82, // Opcode: LMW
/* 6357 */    MCD_OPC_FilterValue, 47, 4, 0, // Skip to: 6365
/* 6361 */    MCD_OPC_Decode, 186, 8, 82, // Opcode: STMW
/* 6365 */    MCD_OPC_FilterValue, 48, 4, 0, // Skip to: 6373
/* 6369 */    MCD_OPC_Decode, 246, 4, 83, // Opcode: LFS
/* 6373 */    MCD_OPC_FilterValue, 49, 4, 0, // Skip to: 6381
/* 6377 */    MCD_OPC_Decode, 247, 4, 83, // Opcode: LFSU
/* 6381 */    MCD_OPC_FilterValue, 50, 4, 0, // Skip to: 6389
/* 6385 */    MCD_OPC_Decode, 240, 4, 84, // Opcode: LFD
/* 6389 */    MCD_OPC_FilterValue, 51, 4, 0, // Skip to: 6397
/* 6393 */    MCD_OPC_Decode, 241, 4, 84, // Opcode: LFDU
/* 6397 */    MCD_OPC_FilterValue, 52, 4, 0, // Skip to: 6405
/* 6401 */    MCD_OPC_Decode, 172, 8, 83, // Opcode: STFS
/* 6405 */    MCD_OPC_FilterValue, 53, 4, 0, // Skip to: 6413
/* 6409 */    MCD_OPC_Decode, 173, 8, 83, // Opcode: STFSU
/* 6413 */    MCD_OPC_FilterValue, 54, 4, 0, // Skip to: 6421
/* 6417 */    MCD_OPC_Decode, 167, 8, 84, // Opcode: STFD
/* 6421 */    MCD_OPC_FilterValue, 55, 4, 0, // Skip to: 6429
/* 6425 */    MCD_OPC_Decode, 168, 8, 84, // Opcode: STFDU
/* 6429 */    MCD_OPC_FilterValue, 58, 27, 0, // Skip to: 6460
/* 6433 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 6436 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6444
/* 6440 */    MCD_OPC_Decode, 226, 4, 85, // Opcode: LD
/* 6444 */    MCD_OPC_FilterValue, 1, 4, 0, // Skip to: 6452
/* 6448 */    MCD_OPC_Decode, 230, 4, 85, // Opcode: LDU
/* 6452 */    MCD_OPC_FilterValue, 2, 189, 13, // Skip to: 9973
/* 6456 */    MCD_OPC_Decode, 154, 5, 85, // Opcode: LWA
/* 6460 */    MCD_OPC_FilterValue, 59, 113, 1, // Skip to: 6833
/* 6464 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 6467 */    MCD_OPC_FilterValue, 28, 31, 0, // Skip to: 6502
/* 6471 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 6474 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 6488
/* 6478 */    MCD_OPC_CheckField, 16, 5, 0, 161, 13, // Skip to: 9973
/* 6484 */    MCD_OPC_Decode, 230, 3, 86, // Opcode: FCFIDS
/* 6488 */    MCD_OPC_FilterValue, 30, 153, 13, // Skip to: 9973
/* 6492 */    MCD_OPC_CheckField, 16, 5, 0, 147, 13, // Skip to: 9973
/* 6498 */    MCD_OPC_Decode, 233, 3, 86, // Opcode: FCFIDUS
/* 6502 */    MCD_OPC_FilterValue, 29, 31, 0, // Skip to: 6537
/* 6506 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 6509 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 6523
/* 6513 */    MCD_OPC_CheckField, 16, 5, 0, 126, 13, // Skip to: 9973
/* 6519 */    MCD_OPC_Decode, 231, 3, 86, // Opcode: FCFIDSo
/* 6523 */    MCD_OPC_FilterValue, 30, 118, 13, // Skip to: 9973
/* 6527 */    MCD_OPC_CheckField, 16, 5, 0, 112, 13, // Skip to: 9973
/* 6533 */    MCD_OPC_Decode, 234, 3, 86, // Opcode: FCFIDUSo
/* 6537 */    MCD_OPC_FilterValue, 36, 10, 0, // Skip to: 6551
/* 6541 */    MCD_OPC_CheckField, 6, 5, 0, 98, 13, // Skip to: 9973
/* 6547 */    MCD_OPC_Decode, 128, 4, 87, // Opcode: FDIVS
/* 6551 */    MCD_OPC_FilterValue, 37, 10, 0, // Skip to: 6565
/* 6555 */    MCD_OPC_CheckField, 6, 5, 0, 84, 13, // Skip to: 9973
/* 6561 */    MCD_OPC_Decode, 129, 4, 87, // Opcode: FDIVSo
/* 6565 */    MCD_OPC_FilterValue, 40, 10, 0, // Skip to: 6579
/* 6569 */    MCD_OPC_CheckField, 6, 5, 0, 70, 13, // Skip to: 9973
/* 6575 */    MCD_OPC_Decode, 196, 4, 87, // Opcode: FSUBS
/* 6579 */    MCD_OPC_FilterValue, 41, 10, 0, // Skip to: 6593
/* 6583 */    MCD_OPC_CheckField, 6, 5, 0, 56, 13, // Skip to: 9973
/* 6589 */    MCD_OPC_Decode, 197, 4, 87, // Opcode: FSUBSo
/* 6593 */    MCD_OPC_FilterValue, 42, 10, 0, // Skip to: 6607
/* 6597 */    MCD_OPC_CheckField, 6, 5, 0, 42, 13, // Skip to: 9973
/* 6603 */    MCD_OPC_Decode, 225, 3, 87, // Opcode: FADDS
/* 6607 */    MCD_OPC_FilterValue, 43, 10, 0, // Skip to: 6621
/* 6611 */    MCD_OPC_CheckField, 6, 5, 0, 28, 13, // Skip to: 9973
/* 6617 */    MCD_OPC_Decode, 226, 3, 87, // Opcode: FADDSo
/* 6621 */    MCD_OPC_FilterValue, 44, 16, 0, // Skip to: 6641
/* 6625 */    MCD_OPC_CheckField, 16, 5, 0, 14, 13, // Skip to: 9973
/* 6631 */    MCD_OPC_CheckField, 6, 5, 0, 8, 13, // Skip to: 9973
/* 6637 */    MCD_OPC_Decode, 192, 4, 88, // Opcode: FSQRTS
/* 6641 */    MCD_OPC_FilterValue, 45, 16, 0, // Skip to: 6661
/* 6645 */    MCD_OPC_CheckField, 16, 5, 0, 250, 12, // Skip to: 9973
/* 6651 */    MCD_OPC_CheckField, 6, 5, 0, 244, 12, // Skip to: 9973
/* 6657 */    MCD_OPC_Decode, 193, 4, 88, // Opcode: FSQRTSo
/* 6661 */    MCD_OPC_FilterValue, 48, 16, 0, // Skip to: 6681
/* 6665 */    MCD_OPC_CheckField, 16, 5, 0, 230, 12, // Skip to: 9973
/* 6671 */    MCD_OPC_CheckField, 6, 5, 0, 224, 12, // Skip to: 9973
/* 6677 */    MCD_OPC_Decode, 162, 4, 88, // Opcode: FRES
/* 6681 */    MCD_OPC_FilterValue, 49, 16, 0, // Skip to: 6701
/* 6685 */    MCD_OPC_CheckField, 16, 5, 0, 210, 12, // Skip to: 9973
/* 6691 */    MCD_OPC_CheckField, 6, 5, 0, 204, 12, // Skip to: 9973
/* 6697 */    MCD_OPC_Decode, 163, 4, 88, // Opcode: FRESo
/* 6701 */    MCD_OPC_FilterValue, 50, 10, 0, // Skip to: 6715
/* 6705 */    MCD_OPC_CheckField, 11, 5, 0, 190, 12, // Skip to: 9973
/* 6711 */    MCD_OPC_Decode, 142, 4, 89, // Opcode: FMULS
/* 6715 */    MCD_OPC_FilterValue, 51, 10, 0, // Skip to: 6729
/* 6719 */    MCD_OPC_CheckField, 11, 5, 0, 176, 12, // Skip to: 9973
/* 6725 */    MCD_OPC_Decode, 143, 4, 89, // Opcode: FMULSo
/* 6729 */    MCD_OPC_FilterValue, 52, 16, 0, // Skip to: 6749
/* 6733 */    MCD_OPC_CheckField, 16, 5, 0, 162, 12, // Skip to: 9973
/* 6739 */    MCD_OPC_CheckField, 6, 5, 0, 156, 12, // Skip to: 9973
/* 6745 */    MCD_OPC_Decode, 184, 4, 88, // Opcode: FRSQRTES
/* 6749 */    MCD_OPC_FilterValue, 53, 16, 0, // Skip to: 6769
/* 6753 */    MCD_OPC_CheckField, 16, 5, 0, 142, 12, // Skip to: 9973
/* 6759 */    MCD_OPC_CheckField, 6, 5, 0, 136, 12, // Skip to: 9973
/* 6765 */    MCD_OPC_Decode, 185, 4, 88, // Opcode: FRSQRTESo
/* 6769 */    MCD_OPC_FilterValue, 56, 4, 0, // Skip to: 6777
/* 6773 */    MCD_OPC_Decode, 138, 4, 90, // Opcode: FMSUBS
/* 6777 */    MCD_OPC_FilterValue, 57, 4, 0, // Skip to: 6785
/* 6781 */    MCD_OPC_Decode, 139, 4, 90, // Opcode: FMSUBSo
/* 6785 */    MCD_OPC_FilterValue, 58, 4, 0, // Skip to: 6793
/* 6789 */    MCD_OPC_Decode, 132, 4, 90, // Opcode: FMADDS
/* 6793 */    MCD_OPC_FilterValue, 59, 4, 0, // Skip to: 6801
/* 6797 */    MCD_OPC_Decode, 133, 4, 90, // Opcode: FMADDSo
/* 6801 */    MCD_OPC_FilterValue, 60, 4, 0, // Skip to: 6809
/* 6805 */    MCD_OPC_Decode, 158, 4, 90, // Opcode: FNMSUBS
/* 6809 */    MCD_OPC_FilterValue, 61, 4, 0, // Skip to: 6817
/* 6813 */    MCD_OPC_Decode, 159, 4, 90, // Opcode: FNMSUBSo
/* 6817 */    MCD_OPC_FilterValue, 62, 4, 0, // Skip to: 6825
/* 6821 */    MCD_OPC_Decode, 154, 4, 90, // Opcode: FNMADDS
/* 6825 */    MCD_OPC_FilterValue, 63, 72, 12, // Skip to: 9973
/* 6829 */    MCD_OPC_Decode, 155, 4, 90, // Opcode: FNMADDSo
/* 6833 */    MCD_OPC_FilterValue, 60, 32, 8, // Skip to: 8917
/* 6837 */    MCD_OPC_ExtractField, 4, 2,  // Inst{5-4} ...
/* 6840 */    MCD_OPC_FilterValue, 0, 16, 2, // Skip to: 7372
/* 6844 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 6847 */    MCD_OPC_FilterValue, 4, 19, 0, // Skip to: 6870
/* 6851 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6854 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6862
/* 6858 */    MCD_OPC_Decode, 220, 10, 91, // Opcode: XSADDDP
/* 6862 */    MCD_OPC_FilterValue, 1, 35, 12, // Skip to: 9973
/* 6866 */    MCD_OPC_Decode, 233, 10, 92, // Opcode: XSMADDADP
/* 6870 */    MCD_OPC_FilterValue, 5, 19, 0, // Skip to: 6893
/* 6874 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6877 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6885
/* 6881 */    MCD_OPC_Decode, 254, 10, 91, // Opcode: XSSUBDP
/* 6885 */    MCD_OPC_FilterValue, 1, 12, 12, // Skip to: 9973
/* 6889 */    MCD_OPC_Decode, 234, 10, 92, // Opcode: XSMADDMDP
/* 6893 */    MCD_OPC_FilterValue, 6, 19, 0, // Skip to: 6916
/* 6897 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6900 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6908
/* 6904 */    MCD_OPC_Decode, 239, 10, 91, // Opcode: XSMULDP
/* 6908 */    MCD_OPC_FilterValue, 1, 245, 11, // Skip to: 9973
/* 6912 */    MCD_OPC_Decode, 237, 10, 92, // Opcode: XSMSUBADP
/* 6916 */    MCD_OPC_FilterValue, 7, 19, 0, // Skip to: 6939
/* 6920 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6923 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6931
/* 6927 */    MCD_OPC_Decode, 232, 10, 91, // Opcode: XSDIVDP
/* 6931 */    MCD_OPC_FilterValue, 1, 222, 11, // Skip to: 9973
/* 6935 */    MCD_OPC_Decode, 238, 10, 92, // Opcode: XSMSUBMDP
/* 6939 */    MCD_OPC_FilterValue, 8, 19, 0, // Skip to: 6962
/* 6943 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6946 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6954
/* 6950 */    MCD_OPC_Decode, 132, 11, 93, // Opcode: XVADDSP
/* 6954 */    MCD_OPC_FilterValue, 1, 199, 11, // Skip to: 9973
/* 6958 */    MCD_OPC_Decode, 168, 11, 94, // Opcode: XVMADDASP
/* 6962 */    MCD_OPC_FilterValue, 9, 19, 0, // Skip to: 6985
/* 6966 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6969 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 6977
/* 6973 */    MCD_OPC_Decode, 210, 11, 93, // Opcode: XVSUBSP
/* 6977 */    MCD_OPC_FilterValue, 1, 176, 11, // Skip to: 9973
/* 6981 */    MCD_OPC_Decode, 170, 11, 94, // Opcode: XVMADDMSP
/* 6985 */    MCD_OPC_FilterValue, 10, 19, 0, // Skip to: 7008
/* 6989 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 6992 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7000
/* 6996 */    MCD_OPC_Decode, 180, 11, 93, // Opcode: XVMULSP
/* 7000 */    MCD_OPC_FilterValue, 1, 153, 11, // Skip to: 9973
/* 7004 */    MCD_OPC_Decode, 176, 11, 94, // Opcode: XVMSUBASP
/* 7008 */    MCD_OPC_FilterValue, 11, 19, 0, // Skip to: 7031
/* 7012 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7015 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7023
/* 7019 */    MCD_OPC_Decode, 166, 11, 93, // Opcode: XVDIVSP
/* 7023 */    MCD_OPC_FilterValue, 1, 130, 11, // Skip to: 9973
/* 7027 */    MCD_OPC_Decode, 178, 11, 94, // Opcode: XVMSUBMSP
/* 7031 */    MCD_OPC_FilterValue, 12, 19, 0, // Skip to: 7054
/* 7035 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7038 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7046
/* 7042 */    MCD_OPC_Decode, 131, 11, 93, // Opcode: XVADDDP
/* 7046 */    MCD_OPC_FilterValue, 1, 107, 11, // Skip to: 9973
/* 7050 */    MCD_OPC_Decode, 167, 11, 94, // Opcode: XVMADDADP
/* 7054 */    MCD_OPC_FilterValue, 13, 19, 0, // Skip to: 7077
/* 7058 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7061 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7069
/* 7065 */    MCD_OPC_Decode, 209, 11, 93, // Opcode: XVSUBDP
/* 7069 */    MCD_OPC_FilterValue, 1, 84, 11, // Skip to: 9973
/* 7073 */    MCD_OPC_Decode, 169, 11, 94, // Opcode: XVMADDMDP
/* 7077 */    MCD_OPC_FilterValue, 14, 19, 0, // Skip to: 7100
/* 7081 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7084 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7092
/* 7088 */    MCD_OPC_Decode, 179, 11, 93, // Opcode: XVMULDP
/* 7092 */    MCD_OPC_FilterValue, 1, 61, 11, // Skip to: 9973
/* 7096 */    MCD_OPC_Decode, 175, 11, 94, // Opcode: XVMSUBADP
/* 7100 */    MCD_OPC_FilterValue, 15, 19, 0, // Skip to: 7123
/* 7104 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7107 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7115
/* 7111 */    MCD_OPC_Decode, 165, 11, 93, // Opcode: XVDIVDP
/* 7115 */    MCD_OPC_FilterValue, 1, 38, 11, // Skip to: 9973
/* 7119 */    MCD_OPC_Decode, 177, 11, 94, // Opcode: XVMSUBMDP
/* 7123 */    MCD_OPC_FilterValue, 20, 19, 0, // Skip to: 7146
/* 7127 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7130 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7138
/* 7134 */    MCD_OPC_Decode, 235, 10, 91, // Opcode: XSMAXDP
/* 7138 */    MCD_OPC_FilterValue, 1, 15, 11, // Skip to: 9973
/* 7142 */    MCD_OPC_Decode, 242, 10, 92, // Opcode: XSNMADDADP
/* 7146 */    MCD_OPC_FilterValue, 21, 19, 0, // Skip to: 7169
/* 7150 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7153 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7161
/* 7157 */    MCD_OPC_Decode, 236, 10, 91, // Opcode: XSMINDP
/* 7161 */    MCD_OPC_FilterValue, 1, 248, 10, // Skip to: 9973
/* 7165 */    MCD_OPC_Decode, 243, 10, 92, // Opcode: XSNMADDMDP
/* 7169 */    MCD_OPC_FilterValue, 22, 19, 0, // Skip to: 7192
/* 7173 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7176 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7184
/* 7180 */    MCD_OPC_Decode, 223, 10, 91, // Opcode: XSCPSGNDP
/* 7184 */    MCD_OPC_FilterValue, 1, 225, 10, // Skip to: 9973
/* 7188 */    MCD_OPC_Decode, 244, 10, 92, // Opcode: XSNMSUBADP
/* 7192 */    MCD_OPC_FilterValue, 23, 10, 0, // Skip to: 7206
/* 7196 */    MCD_OPC_CheckField, 3, 1, 1, 211, 10, // Skip to: 9973
/* 7202 */    MCD_OPC_Decode, 245, 10, 92, // Opcode: XSNMSUBMDP
/* 7206 */    MCD_OPC_FilterValue, 24, 19, 0, // Skip to: 7229
/* 7210 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7213 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7221
/* 7217 */    MCD_OPC_Decode, 172, 11, 93, // Opcode: XVMAXSP
/* 7221 */    MCD_OPC_FilterValue, 1, 188, 10, // Skip to: 9973
/* 7225 */    MCD_OPC_Decode, 186, 11, 94, // Opcode: XVNMADDASP
/* 7229 */    MCD_OPC_FilterValue, 25, 19, 0, // Skip to: 7252
/* 7233 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7236 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7244
/* 7240 */    MCD_OPC_Decode, 174, 11, 93, // Opcode: XVMINSP
/* 7244 */    MCD_OPC_FilterValue, 1, 165, 10, // Skip to: 9973
/* 7248 */    MCD_OPC_Decode, 188, 11, 94, // Opcode: XVNMADDMSP
/* 7252 */    MCD_OPC_FilterValue, 26, 19, 0, // Skip to: 7275
/* 7256 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7259 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7267
/* 7263 */    MCD_OPC_Decode, 146, 11, 93, // Opcode: XVCPSGNSP
/* 7267 */    MCD_OPC_FilterValue, 1, 142, 10, // Skip to: 9973
/* 7271 */    MCD_OPC_Decode, 190, 11, 94, // Opcode: XVNMSUBASP
/* 7275 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 7289
/* 7279 */    MCD_OPC_CheckField, 3, 1, 1, 128, 10, // Skip to: 9973
/* 7285 */    MCD_OPC_Decode, 192, 11, 94, // Opcode: XVNMSUBMSP
/* 7289 */    MCD_OPC_FilterValue, 28, 19, 0, // Skip to: 7312
/* 7293 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7296 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7304
/* 7300 */    MCD_OPC_Decode, 171, 11, 93, // Opcode: XVMAXDP
/* 7304 */    MCD_OPC_FilterValue, 1, 105, 10, // Skip to: 9973
/* 7308 */    MCD_OPC_Decode, 185, 11, 94, // Opcode: XVNMADDADP
/* 7312 */    MCD_OPC_FilterValue, 29, 19, 0, // Skip to: 7335
/* 7316 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7319 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7327
/* 7323 */    MCD_OPC_Decode, 173, 11, 93, // Opcode: XVMINDP
/* 7327 */    MCD_OPC_FilterValue, 1, 82, 10, // Skip to: 9973
/* 7331 */    MCD_OPC_Decode, 187, 11, 94, // Opcode: XVNMADDMDP
/* 7335 */    MCD_OPC_FilterValue, 30, 19, 0, // Skip to: 7358
/* 7339 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7342 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7350
/* 7346 */    MCD_OPC_Decode, 145, 11, 93, // Opcode: XVCPSGNDP
/* 7350 */    MCD_OPC_FilterValue, 1, 59, 10, // Skip to: 9973
/* 7354 */    MCD_OPC_Decode, 189, 11, 94, // Opcode: XVNMSUBADP
/* 7358 */    MCD_OPC_FilterValue, 31, 51, 10, // Skip to: 9973
/* 7362 */    MCD_OPC_CheckField, 3, 1, 1, 45, 10, // Skip to: 9973
/* 7368 */    MCD_OPC_Decode, 191, 11, 94, // Opcode: XVNMSUBMDP
/* 7372 */    MCD_OPC_FilterValue, 1, 130, 1, // Skip to: 7762
/* 7376 */    MCD_OPC_ExtractField, 6, 2,  // Inst{7-6} ...
/* 7379 */    MCD_OPC_FilterValue, 0, 100, 0, // Skip to: 7483
/* 7383 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7386 */    MCD_OPC_FilterValue, 0, 34, 0, // Skip to: 7424
/* 7390 */    MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 7393 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7401
/* 7397 */    MCD_OPC_Decode, 228, 11, 95, // Opcode: XXSLDWI
/* 7401 */    MCD_OPC_FilterValue, 1, 8, 10, // Skip to: 9973
/* 7405 */    MCD_OPC_ExtractField, 8, 2,  // Inst{9-8} ...
/* 7408 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7416
/* 7412 */    MCD_OPC_Decode, 215, 11, 93, // Opcode: XXLAND
/* 7416 */    MCD_OPC_FilterValue, 1, 249, 9, // Skip to: 9973
/* 7420 */    MCD_OPC_Decode, 219, 11, 93, // Opcode: XXLNOR
/* 7424 */    MCD_OPC_FilterValue, 1, 241, 9, // Skip to: 9973
/* 7428 */    MCD_OPC_ExtractField, 8, 3,  // Inst{10-8} ...
/* 7431 */    MCD_OPC_FilterValue, 1, 16, 0, // Skip to: 7451
/* 7435 */    MCD_OPC_CheckField, 21, 2, 0, 228, 9, // Skip to: 9973
/* 7441 */    MCD_OPC_CheckField, 0, 1, 0, 222, 9, // Skip to: 9973
/* 7447 */    MCD_OPC_Decode, 222, 10, 96, // Opcode: XSCMPUDP
/* 7451 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 7459
/* 7455 */    MCD_OPC_Decode, 135, 11, 93, // Opcode: XVCMPEQSP
/* 7459 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 7467
/* 7463 */    MCD_OPC_Decode, 133, 11, 93, // Opcode: XVCMPEQDP
/* 7467 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 7475
/* 7471 */    MCD_OPC_Decode, 136, 11, 93, // Opcode: XVCMPEQSPo
/* 7475 */    MCD_OPC_FilterValue, 7, 190, 9, // Skip to: 9973
/* 7479 */    MCD_OPC_Decode, 134, 11, 93, // Opcode: XVCMPEQDPo
/* 7483 */    MCD_OPC_FilterValue, 1, 100, 0, // Skip to: 7587
/* 7487 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7490 */    MCD_OPC_FilterValue, 0, 34, 0, // Skip to: 7528
/* 7494 */    MCD_OPC_ExtractField, 10, 1,  // Inst{10} ...
/* 7497 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7505
/* 7501 */    MCD_OPC_Decode, 226, 11, 95, // Opcode: XXPERMDI
/* 7505 */    MCD_OPC_FilterValue, 1, 160, 9, // Skip to: 9973
/* 7509 */    MCD_OPC_ExtractField, 8, 2,  // Inst{9-8} ...
/* 7512 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 7520
/* 7516 */    MCD_OPC_Decode, 216, 11, 93, // Opcode: XXLANDC
/* 7520 */    MCD_OPC_FilterValue, 1, 145, 9, // Skip to: 9973
/* 7524 */    MCD_OPC_Decode, 221, 11, 93, // Opcode: XXLORC
/* 7528 */    MCD_OPC_FilterValue, 1, 137, 9, // Skip to: 9973
/* 7532 */    MCD_OPC_ExtractField, 8, 3,  // Inst{10-8} ...
/* 7535 */    MCD_OPC_FilterValue, 1, 16, 0, // Skip to: 7555
/* 7539 */    MCD_OPC_CheckField, 21, 2, 0, 124, 9, // Skip to: 9973
/* 7545 */    MCD_OPC_CheckField, 0, 1, 0, 118, 9, // Skip to: 9973
/* 7551 */    MCD_OPC_Decode, 221, 10, 96, // Opcode: XSCMPODP
/* 7555 */    MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 7563
/* 7559 */    MCD_OPC_Decode, 143, 11, 93, // Opcode: XVCMPGTSP
/* 7563 */    MCD_OPC_FilterValue, 3, 4, 0, // Skip to: 7571
/* 7567 */    MCD_OPC_Decode, 141, 11, 93, // Opcode: XVCMPGTDP
/* 7571 */    MCD_OPC_FilterValue, 6, 4, 0, // Skip to: 7579
/* 7575 */    MCD_OPC_Decode, 144, 11, 93, // Opcode: XVCMPGTSPo
/* 7579 */    MCD_OPC_FilterValue, 7, 86, 9, // Skip to: 9973
/* 7583 */    MCD_OPC_Decode, 142, 11, 93, // Opcode: XVCMPGTDPo
/* 7587 */    MCD_OPC_FilterValue, 2, 136, 0, // Skip to: 7727
/* 7591 */    MCD_OPC_ExtractField, 8, 3,  // Inst{10-8} ...
/* 7594 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 7608
/* 7598 */    MCD_OPC_CheckField, 3, 1, 0, 65, 9, // Skip to: 9973
/* 7604 */    MCD_OPC_Decode, 224, 11, 93, // Opcode: XXMRGHW
/* 7608 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 7622
/* 7612 */    MCD_OPC_CheckField, 3, 1, 0, 51, 9, // Skip to: 9973
/* 7618 */    MCD_OPC_Decode, 225, 11, 93, // Opcode: XXMRGLW
/* 7622 */    MCD_OPC_FilterValue, 2, 31, 0, // Skip to: 7657
/* 7626 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7629 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 7649
/* 7633 */    MCD_OPC_CheckField, 18, 3, 0, 30, 9, // Skip to: 9973
/* 7639 */    MCD_OPC_CheckField, 2, 1, 0, 24, 9, // Skip to: 9973
/* 7645 */    MCD_OPC_Decode, 229, 11, 97, // Opcode: XXSPLTW
/* 7649 */    MCD_OPC_FilterValue, 1, 16, 9, // Skip to: 9973
/* 7653 */    MCD_OPC_Decode, 139, 11, 93, // Opcode: XVCMPGESP
/* 7657 */    MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 7671
/* 7661 */    MCD_OPC_CheckField, 3, 1, 1, 2, 9, // Skip to: 9973
/* 7667 */    MCD_OPC_Decode, 137, 11, 93, // Opcode: XVCMPGEDP
/* 7671 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 7685
/* 7675 */    MCD_OPC_CheckField, 3, 1, 0, 244, 8, // Skip to: 9973
/* 7681 */    MCD_OPC_Decode, 220, 11, 93, // Opcode: XXLOR
/* 7685 */    MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 7699
/* 7689 */    MCD_OPC_CheckField, 3, 1, 0, 230, 8, // Skip to: 9973
/* 7695 */    MCD_OPC_Decode, 218, 11, 93, // Opcode: XXLNAND
/* 7699 */    MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 7713
/* 7703 */    MCD_OPC_CheckField, 3, 1, 1, 216, 8, // Skip to: 9973
/* 7709 */    MCD_OPC_Decode, 140, 11, 93, // Opcode: XVCMPGESPo
/* 7713 */    MCD_OPC_FilterValue, 7, 208, 8, // Skip to: 9973
/* 7717 */    MCD_OPC_CheckField, 3, 1, 1, 202, 8, // Skip to: 9973
/* 7723 */    MCD_OPC_Decode, 138, 11, 93, // Opcode: XVCMPGEDPo
/* 7727 */    MCD_OPC_FilterValue, 3, 194, 8, // Skip to: 9973
/* 7731 */    MCD_OPC_ExtractField, 8, 3,  // Inst{10-8} ...
/* 7734 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 7748
/* 7738 */    MCD_OPC_CheckField, 3, 1, 0, 181, 8, // Skip to: 9973
/* 7744 */    MCD_OPC_Decode, 223, 11, 93, // Opcode: XXLXOR
/* 7748 */    MCD_OPC_FilterValue, 5, 173, 8, // Skip to: 9973
/* 7752 */    MCD_OPC_CheckField, 3, 1, 0, 167, 8, // Skip to: 9973
/* 7758 */    MCD_OPC_Decode, 217, 11, 93, // Opcode: XXLEQV
/* 7762 */    MCD_OPC_FilterValue, 2, 119, 4, // Skip to: 8909
/* 7766 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 7769 */    MCD_OPC_FilterValue, 4, 59, 0, // Skip to: 7832
/* 7773 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 7776 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 7790
/* 7780 */    MCD_OPC_CheckField, 16, 5, 0, 139, 8, // Skip to: 9973
/* 7786 */    MCD_OPC_Decode, 228, 10, 98, // Opcode: XSCVDPUXWS
/* 7790 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 7804
/* 7794 */    MCD_OPC_CheckField, 16, 5, 0, 125, 8, // Skip to: 9973
/* 7800 */    MCD_OPC_Decode, 246, 10, 98, // Opcode: XSRDPI
/* 7804 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 7818
/* 7808 */    MCD_OPC_CheckField, 16, 5, 0, 111, 8, // Skip to: 9973
/* 7814 */    MCD_OPC_Decode, 252, 10, 98, // Opcode: XSRSQRTEDP
/* 7818 */    MCD_OPC_FilterValue, 3, 103, 8, // Skip to: 9973
/* 7822 */    MCD_OPC_CheckField, 16, 5, 0, 97, 8, // Skip to: 9973
/* 7828 */    MCD_OPC_Decode, 253, 10, 98, // Opcode: XSSQRTDP
/* 7832 */    MCD_OPC_FilterValue, 5, 45, 0, // Skip to: 7881
/* 7836 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 7839 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 7853
/* 7843 */    MCD_OPC_CheckField, 16, 5, 0, 76, 8, // Skip to: 9973
/* 7849 */    MCD_OPC_Decode, 226, 10, 98, // Opcode: XSCVDPSXWS
/* 7853 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 7867
/* 7857 */    MCD_OPC_CheckField, 16, 5, 0, 62, 8, // Skip to: 9973
/* 7863 */    MCD_OPC_Decode, 250, 10, 98, // Opcode: XSRDPIZ
/* 7867 */    MCD_OPC_FilterValue, 2, 54, 8, // Skip to: 9973
/* 7871 */    MCD_OPC_CheckField, 16, 5, 0, 48, 8, // Skip to: 9973
/* 7877 */    MCD_OPC_Decode, 251, 10, 98, // Opcode: XSREDP
/* 7881 */    MCD_OPC_FilterValue, 6, 51, 0, // Skip to: 7936
/* 7885 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 7888 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 7902
/* 7892 */    MCD_OPC_CheckField, 16, 5, 0, 27, 8, // Skip to: 9973
/* 7898 */    MCD_OPC_Decode, 249, 10, 98, // Opcode: XSRDPIP
/* 7902 */    MCD_OPC_FilterValue, 2, 16, 0, // Skip to: 7922
/* 7906 */    MCD_OPC_CheckField, 16, 7, 0, 13, 8, // Skip to: 9973
/* 7912 */    MCD_OPC_CheckField, 0, 1, 0, 7, 8, // Skip to: 9973
/* 7918 */    MCD_OPC_Decode, 128, 11, 99, // Opcode: XSTSQRTDP
/* 7922 */    MCD_OPC_FilterValue, 3, 255, 7, // Skip to: 9973
/* 7926 */    MCD_OPC_CheckField, 16, 5, 0, 249, 7, // Skip to: 9973
/* 7932 */    MCD_OPC_Decode, 247, 10, 98, // Opcode: XSRDPIC
/* 7936 */    MCD_OPC_FilterValue, 7, 43, 0, // Skip to: 7983
/* 7940 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 7943 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 7963
/* 7947 */    MCD_OPC_CheckField, 16, 5, 0, 228, 7, // Skip to: 9973
/* 7953 */    MCD_OPC_CheckField, 2, 1, 1, 222, 7, // Skip to: 9973
/* 7959 */    MCD_OPC_Decode, 248, 10, 98, // Opcode: XSRDPIM
/* 7963 */    MCD_OPC_FilterValue, 1, 214, 7, // Skip to: 9973
/* 7967 */    MCD_OPC_CheckField, 21, 2, 0, 208, 7, // Skip to: 9973
/* 7973 */    MCD_OPC_CheckField, 0, 1, 0, 202, 7, // Skip to: 9973
/* 7979 */    MCD_OPC_Decode, 255, 10, 96, // Opcode: XSTDIVDP
/* 7983 */    MCD_OPC_FilterValue, 8, 59, 0, // Skip to: 8046
/* 7987 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 7990 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8004
/* 7994 */    MCD_OPC_CheckField, 16, 5, 0, 181, 7, // Skip to: 9973
/* 8000 */    MCD_OPC_Decode, 156, 11, 100, // Opcode: XVCVSPUXWS
/* 8004 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8018
/* 8008 */    MCD_OPC_CheckField, 16, 5, 0, 167, 7, // Skip to: 9973
/* 8014 */    MCD_OPC_Decode, 200, 11, 100, // Opcode: XVRSPI
/* 8018 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 8032
/* 8022 */    MCD_OPC_CheckField, 16, 5, 0, 153, 7, // Skip to: 9973
/* 8028 */    MCD_OPC_Decode, 206, 11, 100, // Opcode: XVRSQRTESP
/* 8032 */    MCD_OPC_FilterValue, 3, 145, 7, // Skip to: 9973
/* 8036 */    MCD_OPC_CheckField, 16, 5, 0, 139, 7, // Skip to: 9973
/* 8042 */    MCD_OPC_Decode, 208, 11, 100, // Opcode: XVSQRTSP
/* 8046 */    MCD_OPC_FilterValue, 9, 45, 0, // Skip to: 8095
/* 8050 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8053 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8067
/* 8057 */    MCD_OPC_CheckField, 16, 5, 0, 118, 7, // Skip to: 9973
/* 8063 */    MCD_OPC_Decode, 154, 11, 100, // Opcode: XVCVSPSXWS
/* 8067 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8081
/* 8071 */    MCD_OPC_CheckField, 16, 5, 0, 104, 7, // Skip to: 9973
/* 8077 */    MCD_OPC_Decode, 204, 11, 100, // Opcode: XVRSPIZ
/* 8081 */    MCD_OPC_FilterValue, 2, 96, 7, // Skip to: 9973
/* 8085 */    MCD_OPC_CheckField, 16, 5, 0, 90, 7, // Skip to: 9973
/* 8091 */    MCD_OPC_Decode, 199, 11, 100, // Opcode: XVRESP
/* 8095 */    MCD_OPC_FilterValue, 10, 65, 0, // Skip to: 8164
/* 8099 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8102 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8116
/* 8106 */    MCD_OPC_CheckField, 16, 5, 0, 69, 7, // Skip to: 9973
/* 8112 */    MCD_OPC_Decode, 164, 11, 100, // Opcode: XVCVUXWSP
/* 8116 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8130
/* 8120 */    MCD_OPC_CheckField, 16, 5, 0, 55, 7, // Skip to: 9973
/* 8126 */    MCD_OPC_Decode, 203, 11, 100, // Opcode: XVRSPIP
/* 8130 */    MCD_OPC_FilterValue, 2, 16, 0, // Skip to: 8150
/* 8134 */    MCD_OPC_CheckField, 16, 7, 0, 41, 7, // Skip to: 9973
/* 8140 */    MCD_OPC_CheckField, 0, 1, 0, 35, 7, // Skip to: 9973
/* 8146 */    MCD_OPC_Decode, 214, 11, 101, // Opcode: XVTSQRTSP
/* 8150 */    MCD_OPC_FilterValue, 3, 27, 7, // Skip to: 9973
/* 8154 */    MCD_OPC_CheckField, 16, 5, 0, 21, 7, // Skip to: 9973
/* 8160 */    MCD_OPC_Decode, 201, 11, 100, // Opcode: XVRSPIC
/* 8164 */    MCD_OPC_FilterValue, 11, 58, 0, // Skip to: 8226
/* 8168 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 8171 */    MCD_OPC_FilterValue, 0, 31, 0, // Skip to: 8206
/* 8175 */    MCD_OPC_ExtractField, 2, 1,  // Inst{2} ...
/* 8178 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8192
/* 8182 */    MCD_OPC_CheckField, 16, 5, 0, 249, 6, // Skip to: 9973
/* 8188 */    MCD_OPC_Decode, 160, 11, 100, // Opcode: XVCVSXWSP
/* 8192 */    MCD_OPC_FilterValue, 1, 241, 6, // Skip to: 9973
/* 8196 */    MCD_OPC_CheckField, 16, 5, 0, 235, 6, // Skip to: 9973
/* 8202 */    MCD_OPC_Decode, 202, 11, 100, // Opcode: XVRSPIM
/* 8206 */    MCD_OPC_FilterValue, 1, 227, 6, // Skip to: 9973
/* 8210 */    MCD_OPC_CheckField, 21, 2, 0, 221, 6, // Skip to: 9973
/* 8216 */    MCD_OPC_CheckField, 0, 1, 0, 215, 6, // Skip to: 9973
/* 8222 */    MCD_OPC_Decode, 212, 11, 102, // Opcode: XVTDIVSP
/* 8226 */    MCD_OPC_FilterValue, 12, 59, 0, // Skip to: 8289
/* 8230 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8233 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8247
/* 8237 */    MCD_OPC_CheckField, 16, 5, 0, 194, 6, // Skip to: 9973
/* 8243 */    MCD_OPC_Decode, 151, 11, 100, // Opcode: XVCVDPUXWS
/* 8247 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8261
/* 8251 */    MCD_OPC_CheckField, 16, 5, 0, 180, 6, // Skip to: 9973
/* 8257 */    MCD_OPC_Decode, 193, 11, 100, // Opcode: XVRDPI
/* 8261 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 8275
/* 8265 */    MCD_OPC_CheckField, 16, 5, 0, 166, 6, // Skip to: 9973
/* 8271 */    MCD_OPC_Decode, 205, 11, 100, // Opcode: XVRSQRTEDP
/* 8275 */    MCD_OPC_FilterValue, 3, 158, 6, // Skip to: 9973
/* 8279 */    MCD_OPC_CheckField, 16, 5, 0, 152, 6, // Skip to: 9973
/* 8285 */    MCD_OPC_Decode, 207, 11, 100, // Opcode: XVSQRTDP
/* 8289 */    MCD_OPC_FilterValue, 13, 45, 0, // Skip to: 8338
/* 8293 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8296 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8310
/* 8300 */    MCD_OPC_CheckField, 16, 5, 0, 131, 6, // Skip to: 9973
/* 8306 */    MCD_OPC_Decode, 149, 11, 100, // Opcode: XVCVDPSXWS
/* 8310 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8324
/* 8314 */    MCD_OPC_CheckField, 16, 5, 0, 117, 6, // Skip to: 9973
/* 8320 */    MCD_OPC_Decode, 197, 11, 100, // Opcode: XVRDPIZ
/* 8324 */    MCD_OPC_FilterValue, 2, 109, 6, // Skip to: 9973
/* 8328 */    MCD_OPC_CheckField, 16, 5, 0, 103, 6, // Skip to: 9973
/* 8334 */    MCD_OPC_Decode, 198, 11, 100, // Opcode: XVREDP
/* 8338 */    MCD_OPC_FilterValue, 14, 65, 0, // Skip to: 8407
/* 8342 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8345 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8359
/* 8349 */    MCD_OPC_CheckField, 16, 5, 0, 82, 6, // Skip to: 9973
/* 8355 */    MCD_OPC_Decode, 163, 11, 100, // Opcode: XVCVUXWDP
/* 8359 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 8373
/* 8363 */    MCD_OPC_CheckField, 16, 5, 0, 68, 6, // Skip to: 9973
/* 8369 */    MCD_OPC_Decode, 196, 11, 100, // Opcode: XVRDPIP
/* 8373 */    MCD_OPC_FilterValue, 2, 16, 0, // Skip to: 8393
/* 8377 */    MCD_OPC_CheckField, 16, 7, 0, 54, 6, // Skip to: 9973
/* 8383 */    MCD_OPC_CheckField, 0, 1, 0, 48, 6, // Skip to: 9973
/* 8389 */    MCD_OPC_Decode, 213, 11, 101, // Opcode: XVTSQRTDP
/* 8393 */    MCD_OPC_FilterValue, 3, 40, 6, // Skip to: 9973
/* 8397 */    MCD_OPC_CheckField, 16, 5, 0, 34, 6, // Skip to: 9973
/* 8403 */    MCD_OPC_Decode, 194, 11, 100, // Opcode: XVRDPIC
/* 8407 */    MCD_OPC_FilterValue, 15, 58, 0, // Skip to: 8469
/* 8411 */    MCD_OPC_ExtractField, 3, 1,  // Inst{3} ...
/* 8414 */    MCD_OPC_FilterValue, 0, 31, 0, // Skip to: 8449
/* 8418 */    MCD_OPC_ExtractField, 2, 1,  // Inst{2} ...
/* 8421 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8435
/* 8425 */    MCD_OPC_CheckField, 16, 5, 0, 6, 6, // Skip to: 9973
/* 8431 */    MCD_OPC_Decode, 159, 11, 100, // Opcode: XVCVSXWDP
/* 8435 */    MCD_OPC_FilterValue, 1, 254, 5, // Skip to: 9973
/* 8439 */    MCD_OPC_CheckField, 16, 5, 0, 248, 5, // Skip to: 9973
/* 8445 */    MCD_OPC_Decode, 195, 11, 100, // Opcode: XVRDPIM
/* 8449 */    MCD_OPC_FilterValue, 1, 240, 5, // Skip to: 9973
/* 8453 */    MCD_OPC_CheckField, 21, 2, 0, 234, 5, // Skip to: 9973
/* 8459 */    MCD_OPC_CheckField, 0, 1, 0, 228, 5, // Skip to: 9973
/* 8465 */    MCD_OPC_Decode, 211, 11, 102, // Opcode: XVTDIVDP
/* 8469 */    MCD_OPC_FilterValue, 16, 16, 0, // Skip to: 8489
/* 8473 */    MCD_OPC_CheckField, 16, 5, 0, 214, 5, // Skip to: 9973
/* 8479 */    MCD_OPC_CheckField, 2, 2, 1, 208, 5, // Skip to: 9973
/* 8485 */    MCD_OPC_Decode, 224, 10, 98, // Opcode: XSCVDPSP
/* 8489 */    MCD_OPC_FilterValue, 20, 31, 0, // Skip to: 8524
/* 8493 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8496 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8510
/* 8500 */    MCD_OPC_CheckField, 16, 5, 0, 187, 5, // Skip to: 9973
/* 8506 */    MCD_OPC_Decode, 227, 10, 98, // Opcode: XSCVDPUXDS
/* 8510 */    MCD_OPC_FilterValue, 1, 179, 5, // Skip to: 9973
/* 8514 */    MCD_OPC_CheckField, 16, 5, 0, 173, 5, // Skip to: 9973
/* 8520 */    MCD_OPC_Decode, 229, 10, 98, // Opcode: XSCVSPDP
/* 8524 */    MCD_OPC_FilterValue, 21, 31, 0, // Skip to: 8559
/* 8528 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8531 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8545
/* 8535 */    MCD_OPC_CheckField, 16, 5, 0, 152, 5, // Skip to: 9973
/* 8541 */    MCD_OPC_Decode, 225, 10, 98, // Opcode: XSCVDPSXDS
/* 8545 */    MCD_OPC_FilterValue, 1, 144, 5, // Skip to: 9973
/* 8549 */    MCD_OPC_CheckField, 16, 5, 0, 138, 5, // Skip to: 9973
/* 8555 */    MCD_OPC_Decode, 219, 10, 98, // Opcode: XSABSDP
/* 8559 */    MCD_OPC_FilterValue, 22, 31, 0, // Skip to: 8594
/* 8563 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8566 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8580
/* 8570 */    MCD_OPC_CheckField, 16, 5, 0, 117, 5, // Skip to: 9973
/* 8576 */    MCD_OPC_Decode, 231, 10, 98, // Opcode: XSCVUXDDP
/* 8580 */    MCD_OPC_FilterValue, 1, 109, 5, // Skip to: 9973
/* 8584 */    MCD_OPC_CheckField, 16, 5, 0, 103, 5, // Skip to: 9973
/* 8590 */    MCD_OPC_Decode, 240, 10, 98, // Opcode: XSNABSDP
/* 8594 */    MCD_OPC_FilterValue, 23, 31, 0, // Skip to: 8629
/* 8598 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8601 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8615
/* 8605 */    MCD_OPC_CheckField, 16, 5, 0, 82, 5, // Skip to: 9973
/* 8611 */    MCD_OPC_Decode, 230, 10, 98, // Opcode: XSCVSXDDP
/* 8615 */    MCD_OPC_FilterValue, 1, 74, 5, // Skip to: 9973
/* 8619 */    MCD_OPC_CheckField, 16, 5, 0, 68, 5, // Skip to: 9973
/* 8625 */    MCD_OPC_Decode, 241, 10, 98, // Opcode: XSNEGDP
/* 8629 */    MCD_OPC_FilterValue, 24, 31, 0, // Skip to: 8664
/* 8633 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8636 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8650
/* 8640 */    MCD_OPC_CheckField, 16, 5, 0, 47, 5, // Skip to: 9973
/* 8646 */    MCD_OPC_Decode, 155, 11, 100, // Opcode: XVCVSPUXDS
/* 8650 */    MCD_OPC_FilterValue, 1, 39, 5, // Skip to: 9973
/* 8654 */    MCD_OPC_CheckField, 16, 5, 0, 33, 5, // Skip to: 9973
/* 8660 */    MCD_OPC_Decode, 147, 11, 100, // Opcode: XVCVDPSP
/* 8664 */    MCD_OPC_FilterValue, 25, 31, 0, // Skip to: 8699
/* 8668 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8671 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8685
/* 8675 */    MCD_OPC_CheckField, 16, 5, 0, 12, 5, // Skip to: 9973
/* 8681 */    MCD_OPC_Decode, 153, 11, 100, // Opcode: XVCVSPSXDS
/* 8685 */    MCD_OPC_FilterValue, 1, 4, 5, // Skip to: 9973
/* 8689 */    MCD_OPC_CheckField, 16, 5, 0, 254, 4, // Skip to: 9973
/* 8695 */    MCD_OPC_Decode, 130, 11, 100, // Opcode: XVABSSP
/* 8699 */    MCD_OPC_FilterValue, 26, 31, 0, // Skip to: 8734
/* 8703 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8706 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8720
/* 8710 */    MCD_OPC_CheckField, 16, 5, 0, 233, 4, // Skip to: 9973
/* 8716 */    MCD_OPC_Decode, 162, 11, 100, // Opcode: XVCVUXDSP
/* 8720 */    MCD_OPC_FilterValue, 1, 225, 4, // Skip to: 9973
/* 8724 */    MCD_OPC_CheckField, 16, 5, 0, 219, 4, // Skip to: 9973
/* 8730 */    MCD_OPC_Decode, 182, 11, 100, // Opcode: XVNABSSP
/* 8734 */    MCD_OPC_FilterValue, 27, 31, 0, // Skip to: 8769
/* 8738 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8741 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8755
/* 8745 */    MCD_OPC_CheckField, 16, 5, 0, 198, 4, // Skip to: 9973
/* 8751 */    MCD_OPC_Decode, 158, 11, 100, // Opcode: XVCVSXDSP
/* 8755 */    MCD_OPC_FilterValue, 1, 190, 4, // Skip to: 9973
/* 8759 */    MCD_OPC_CheckField, 16, 5, 0, 184, 4, // Skip to: 9973
/* 8765 */    MCD_OPC_Decode, 184, 11, 100, // Opcode: XVNEGSP
/* 8769 */    MCD_OPC_FilterValue, 28, 31, 0, // Skip to: 8804
/* 8773 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8776 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8790
/* 8780 */    MCD_OPC_CheckField, 16, 5, 0, 163, 4, // Skip to: 9973
/* 8786 */    MCD_OPC_Decode, 150, 11, 100, // Opcode: XVCVDPUXDS
/* 8790 */    MCD_OPC_FilterValue, 1, 155, 4, // Skip to: 9973
/* 8794 */    MCD_OPC_CheckField, 16, 5, 0, 149, 4, // Skip to: 9973
/* 8800 */    MCD_OPC_Decode, 152, 11, 100, // Opcode: XVCVSPDP
/* 8804 */    MCD_OPC_FilterValue, 29, 31, 0, // Skip to: 8839
/* 8808 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8811 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8825
/* 8815 */    MCD_OPC_CheckField, 16, 5, 0, 128, 4, // Skip to: 9973
/* 8821 */    MCD_OPC_Decode, 148, 11, 100, // Opcode: XVCVDPSXDS
/* 8825 */    MCD_OPC_FilterValue, 1, 120, 4, // Skip to: 9973
/* 8829 */    MCD_OPC_CheckField, 16, 5, 0, 114, 4, // Skip to: 9973
/* 8835 */    MCD_OPC_Decode, 129, 11, 100, // Opcode: XVABSDP
/* 8839 */    MCD_OPC_FilterValue, 30, 31, 0, // Skip to: 8874
/* 8843 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8846 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8860
/* 8850 */    MCD_OPC_CheckField, 16, 5, 0, 93, 4, // Skip to: 9973
/* 8856 */    MCD_OPC_Decode, 161, 11, 100, // Opcode: XVCVUXDDP
/* 8860 */    MCD_OPC_FilterValue, 1, 85, 4, // Skip to: 9973
/* 8864 */    MCD_OPC_CheckField, 16, 5, 0, 79, 4, // Skip to: 9973
/* 8870 */    MCD_OPC_Decode, 181, 11, 100, // Opcode: XVNABSDP
/* 8874 */    MCD_OPC_FilterValue, 31, 71, 4, // Skip to: 9973
/* 8878 */    MCD_OPC_ExtractField, 2, 2,  // Inst{3-2} ...
/* 8881 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8895
/* 8885 */    MCD_OPC_CheckField, 16, 5, 0, 58, 4, // Skip to: 9973
/* 8891 */    MCD_OPC_Decode, 157, 11, 100, // Opcode: XVCVSXDDP
/* 8895 */    MCD_OPC_FilterValue, 1, 50, 4, // Skip to: 9973
/* 8899 */    MCD_OPC_CheckField, 16, 5, 0, 44, 4, // Skip to: 9973
/* 8905 */    MCD_OPC_Decode, 183, 11, 100, // Opcode: XVNEGDP
/* 8909 */    MCD_OPC_FilterValue, 3, 36, 4, // Skip to: 9973
/* 8913 */    MCD_OPC_Decode, 227, 11, 103, // Opcode: XXSEL
/* 8917 */    MCD_OPC_FilterValue, 62, 19, 0, // Skip to: 8940
/* 8921 */    MCD_OPC_ExtractField, 0, 2,  // Inst{1-0} ...
/* 8924 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 8932
/* 8928 */    MCD_OPC_Decode, 160, 8, 85, // Opcode: STD
/* 8932 */    MCD_OPC_FilterValue, 1, 13, 4, // Skip to: 9973
/* 8936 */    MCD_OPC_Decode, 164, 8, 85, // Opcode: STDU
/* 8940 */    MCD_OPC_FilterValue, 63, 5, 4, // Skip to: 9973
/* 8944 */    MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 8947 */    MCD_OPC_FilterValue, 0, 37, 0, // Skip to: 8988
/* 8951 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 8954 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 8968
/* 8958 */    MCD_OPC_CheckField, 21, 2, 0, 241, 3, // Skip to: 9973
/* 8964 */    MCD_OPC_Decode, 238, 3, 104, // Opcode: FCMPUS
/* 8968 */    MCD_OPC_FilterValue, 2, 233, 3, // Skip to: 9973
/* 8972 */    MCD_OPC_CheckField, 21, 2, 0, 227, 3, // Skip to: 9973
/* 8978 */    MCD_OPC_CheckField, 11, 7, 0, 221, 3, // Skip to: 9973
/* 8984 */    MCD_OPC_Decode, 178, 5, 23, // Opcode: MCRFS
/* 8988 */    MCD_OPC_FilterValue, 12, 45, 0, // Skip to: 9037
/* 8992 */    MCD_OPC_ExtractField, 6, 6,  // Inst{11-6} ...
/* 8995 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 9009
/* 8999 */    MCD_OPC_CheckField, 12, 9, 0, 200, 3, // Skip to: 9973
/* 9005 */    MCD_OPC_Decode, 208, 5, 75, // Opcode: MTFSB1
/* 9009 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 9023
/* 9013 */    MCD_OPC_CheckField, 12, 9, 0, 186, 3, // Skip to: 9973
/* 9019 */    MCD_OPC_Decode, 207, 5, 75, // Opcode: MTFSB0
/* 9023 */    MCD_OPC_FilterValue, 4, 178, 3, // Skip to: 9973
/* 9027 */    MCD_OPC_CheckField, 17, 6, 0, 172, 3, // Skip to: 9973
/* 9033 */    MCD_OPC_Decode, 210, 5, 105, // Opcode: MTFSFI
/* 9037 */    MCD_OPC_FilterValue, 13, 16, 0, // Skip to: 9057
/* 9041 */    MCD_OPC_CheckField, 17, 6, 0, 158, 3, // Skip to: 9973
/* 9047 */    MCD_OPC_CheckField, 6, 6, 4, 152, 3, // Skip to: 9973
/* 9053 */    MCD_OPC_Decode, 211, 5, 105, // Opcode: MTFSFIo
/* 9057 */    MCD_OPC_FilterValue, 14, 25, 0, // Skip to: 9086
/* 9061 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9064 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 9078
/* 9068 */    MCD_OPC_CheckField, 11, 10, 0, 131, 3, // Skip to: 9973
/* 9074 */    MCD_OPC_Decode, 184, 5, 106, // Opcode: MFFS
/* 9078 */    MCD_OPC_FilterValue, 22, 123, 3, // Skip to: 9973
/* 9082 */    MCD_OPC_Decode, 209, 5, 107, // Opcode: MTFSF
/* 9086 */    MCD_OPC_FilterValue, 15, 25, 0, // Skip to: 9115
/* 9090 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9093 */    MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 9107
/* 9097 */    MCD_OPC_CheckField, 11, 10, 0, 102, 3, // Skip to: 9973
/* 9103 */    MCD_OPC_Decode, 185, 5, 106, // Opcode: MFFSo
/* 9107 */    MCD_OPC_FilterValue, 22, 94, 3, // Skip to: 9973
/* 9111 */    MCD_OPC_Decode, 213, 5, 107, // Opcode: MTFSFo
/* 9115 */    MCD_OPC_FilterValue, 16, 123, 0, // Skip to: 9242
/* 9119 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9122 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 9130
/* 9126 */    MCD_OPC_Decode, 241, 3, 87, // Opcode: FCPSGNS
/* 9130 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 9144
/* 9134 */    MCD_OPC_CheckField, 16, 5, 0, 65, 3, // Skip to: 9973
/* 9140 */    MCD_OPC_Decode, 151, 4, 88, // Opcode: FNEGS
/* 9144 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 9158
/* 9148 */    MCD_OPC_CheckField, 16, 5, 0, 51, 3, // Skip to: 9973
/* 9154 */    MCD_OPC_Decode, 135, 4, 88, // Opcode: FMR
/* 9158 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 9172
/* 9162 */    MCD_OPC_CheckField, 16, 5, 0, 37, 3, // Skip to: 9973
/* 9168 */    MCD_OPC_Decode, 147, 4, 88, // Opcode: FNABSS
/* 9172 */    MCD_OPC_FilterValue, 8, 10, 0, // Skip to: 9186
/* 9176 */    MCD_OPC_CheckField, 16, 5, 0, 23, 3, // Skip to: 9973
/* 9182 */    MCD_OPC_Decode, 222, 3, 88, // Opcode: FABSS
/* 9186 */    MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 9200
/* 9190 */    MCD_OPC_CheckField, 16, 5, 0, 9, 3, // Skip to: 9973
/* 9196 */    MCD_OPC_Decode, 171, 4, 88, // Opcode: FRINS
/* 9200 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 9214
/* 9204 */    MCD_OPC_CheckField, 16, 5, 0, 251, 2, // Skip to: 9973
/* 9210 */    MCD_OPC_Decode, 179, 4, 88, // Opcode: FRIZS
/* 9214 */    MCD_OPC_FilterValue, 14, 10, 0, // Skip to: 9228
/* 9218 */    MCD_OPC_CheckField, 16, 5, 0, 237, 2, // Skip to: 9973
/* 9224 */    MCD_OPC_Decode, 175, 4, 88, // Opcode: FRIPS
/* 9228 */    MCD_OPC_FilterValue, 15, 229, 2, // Skip to: 9973
/* 9232 */    MCD_OPC_CheckField, 16, 5, 0, 223, 2, // Skip to: 9973
/* 9238 */    MCD_OPC_Decode, 167, 4, 88, // Opcode: FRIMS
/* 9242 */    MCD_OPC_FilterValue, 17, 123, 0, // Skip to: 9369
/* 9246 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9249 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 9257
/* 9253 */    MCD_OPC_Decode, 242, 3, 87, // Opcode: FCPSGNSo
/* 9257 */    MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 9271
/* 9261 */    MCD_OPC_CheckField, 16, 5, 0, 194, 2, // Skip to: 9973
/* 9267 */    MCD_OPC_Decode, 152, 4, 88, // Opcode: FNEGSo
/* 9271 */    MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 9285
/* 9275 */    MCD_OPC_CheckField, 16, 5, 0, 180, 2, // Skip to: 9973
/* 9281 */    MCD_OPC_Decode, 136, 4, 88, // Opcode: FMRo
/* 9285 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 9299
/* 9289 */    MCD_OPC_CheckField, 16, 5, 0, 166, 2, // Skip to: 9973
/* 9295 */    MCD_OPC_Decode, 148, 4, 88, // Opcode: FNABSSo
/* 9299 */    MCD_OPC_FilterValue, 8, 10, 0, // Skip to: 9313
/* 9303 */    MCD_OPC_CheckField, 16, 5, 0, 152, 2, // Skip to: 9973
/* 9309 */    MCD_OPC_Decode, 223, 3, 88, // Opcode: FABSSo
/* 9313 */    MCD_OPC_FilterValue, 12, 10, 0, // Skip to: 9327
/* 9317 */    MCD_OPC_CheckField, 16, 5, 0, 138, 2, // Skip to: 9973
/* 9323 */    MCD_OPC_Decode, 172, 4, 88, // Opcode: FRINSo
/* 9327 */    MCD_OPC_FilterValue, 13, 10, 0, // Skip to: 9341
/* 9331 */    MCD_OPC_CheckField, 16, 5, 0, 124, 2, // Skip to: 9973
/* 9337 */    MCD_OPC_Decode, 180, 4, 88, // Opcode: FRIZSo
/* 9341 */    MCD_OPC_FilterValue, 14, 10, 0, // Skip to: 9355
/* 9345 */    MCD_OPC_CheckField, 16, 5, 0, 110, 2, // Skip to: 9973
/* 9351 */    MCD_OPC_Decode, 176, 4, 88, // Opcode: FRIPSo
/* 9355 */    MCD_OPC_FilterValue, 15, 102, 2, // Skip to: 9973
/* 9359 */    MCD_OPC_CheckField, 16, 5, 0, 96, 2, // Skip to: 9973
/* 9365 */    MCD_OPC_Decode, 168, 4, 88, // Opcode: FRIMSo
/* 9369 */    MCD_OPC_FilterValue, 24, 16, 0, // Skip to: 9389
/* 9373 */    MCD_OPC_CheckField, 16, 5, 0, 82, 2, // Skip to: 9973
/* 9379 */    MCD_OPC_CheckField, 6, 5, 0, 76, 2, // Skip to: 9973
/* 9385 */    MCD_OPC_Decode, 181, 4, 86, // Opcode: FRSP
/* 9389 */    MCD_OPC_FilterValue, 25, 16, 0, // Skip to: 9409
/* 9393 */    MCD_OPC_CheckField, 16, 5, 0, 62, 2, // Skip to: 9973
/* 9399 */    MCD_OPC_CheckField, 6, 5, 0, 56, 2, // Skip to: 9973
/* 9405 */    MCD_OPC_Decode, 182, 4, 86, // Opcode: FRSPo
/* 9409 */    MCD_OPC_FilterValue, 28, 59, 0, // Skip to: 9472
/* 9413 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9416 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 9430
/* 9420 */    MCD_OPC_CheckField, 16, 5, 0, 35, 2, // Skip to: 9973
/* 9426 */    MCD_OPC_Decode, 249, 3, 108, // Opcode: FCTIW
/* 9430 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 9444
/* 9434 */    MCD_OPC_CheckField, 16, 5, 0, 21, 2, // Skip to: 9973
/* 9440 */    MCD_OPC_Decode, 243, 3, 108, // Opcode: FCTID
/* 9444 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 9458
/* 9448 */    MCD_OPC_CheckField, 16, 5, 0, 7, 2, // Skip to: 9973
/* 9454 */    MCD_OPC_Decode, 229, 3, 108, // Opcode: FCFID
/* 9458 */    MCD_OPC_FilterValue, 30, 255, 1, // Skip to: 9973
/* 9462 */    MCD_OPC_CheckField, 16, 5, 0, 249, 1, // Skip to: 9973
/* 9468 */    MCD_OPC_Decode, 232, 3, 108, // Opcode: FCFIDU
/* 9472 */    MCD_OPC_FilterValue, 29, 59, 0, // Skip to: 9535
/* 9476 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9479 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 9493
/* 9483 */    MCD_OPC_CheckField, 16, 5, 0, 228, 1, // Skip to: 9973
/* 9489 */    MCD_OPC_Decode, 254, 3, 108, // Opcode: FCTIWo
/* 9493 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 9507
/* 9497 */    MCD_OPC_CheckField, 16, 5, 0, 214, 1, // Skip to: 9973
/* 9503 */    MCD_OPC_Decode, 248, 3, 108, // Opcode: FCTIDo
/* 9507 */    MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 9521
/* 9511 */    MCD_OPC_CheckField, 16, 5, 0, 200, 1, // Skip to: 9973
/* 9517 */    MCD_OPC_Decode, 236, 3, 108, // Opcode: FCFIDo
/* 9521 */    MCD_OPC_FilterValue, 30, 192, 1, // Skip to: 9973
/* 9525 */    MCD_OPC_CheckField, 16, 5, 0, 186, 1, // Skip to: 9973
/* 9531 */    MCD_OPC_Decode, 235, 3, 108, // Opcode: FCFIDUo
/* 9535 */    MCD_OPC_FilterValue, 30, 59, 0, // Skip to: 9598
/* 9539 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9542 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 9556
/* 9546 */    MCD_OPC_CheckField, 16, 5, 0, 165, 1, // Skip to: 9973
/* 9552 */    MCD_OPC_Decode, 252, 3, 108, // Opcode: FCTIWZ
/* 9556 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 9570
/* 9560 */    MCD_OPC_CheckField, 16, 5, 0, 151, 1, // Skip to: 9973
/* 9566 */    MCD_OPC_Decode, 250, 3, 108, // Opcode: FCTIWUZ
/* 9570 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 9584
/* 9574 */    MCD_OPC_CheckField, 16, 5, 0, 137, 1, // Skip to: 9973
/* 9580 */    MCD_OPC_Decode, 246, 3, 108, // Opcode: FCTIDZ
/* 9584 */    MCD_OPC_FilterValue, 29, 129, 1, // Skip to: 9973
/* 9588 */    MCD_OPC_CheckField, 16, 5, 0, 123, 1, // Skip to: 9973
/* 9594 */    MCD_OPC_Decode, 244, 3, 108, // Opcode: FCTIDUZ
/* 9598 */    MCD_OPC_FilterValue, 31, 59, 0, // Skip to: 9661
/* 9602 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 9605 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 9619
/* 9609 */    MCD_OPC_CheckField, 16, 5, 0, 102, 1, // Skip to: 9973
/* 9615 */    MCD_OPC_Decode, 253, 3, 108, // Opcode: FCTIWZo
/* 9619 */    MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 9633
/* 9623 */    MCD_OPC_CheckField, 16, 5, 0, 88, 1, // Skip to: 9973
/* 9629 */    MCD_OPC_Decode, 251, 3, 108, // Opcode: FCTIWUZo
/* 9633 */    MCD_OPC_FilterValue, 25, 10, 0, // Skip to: 9647
/* 9637 */    MCD_OPC_CheckField, 16, 5, 0, 74, 1, // Skip to: 9973
/* 9643 */    MCD_OPC_Decode, 247, 3, 108, // Opcode: FCTIDZo
/* 9647 */    MCD_OPC_FilterValue, 29, 66, 1, // Skip to: 9973
/* 9651 */    MCD_OPC_CheckField, 16, 5, 0, 60, 1, // Skip to: 9973
/* 9657 */    MCD_OPC_Decode, 245, 3, 108, // Opcode: FCTIDUZo
/* 9661 */    MCD_OPC_FilterValue, 36, 10, 0, // Skip to: 9675
/* 9665 */    MCD_OPC_CheckField, 6, 5, 0, 46, 1, // Skip to: 9973
/* 9671 */    MCD_OPC_Decode, 255, 3, 109, // Opcode: FDIV
/* 9675 */    MCD_OPC_FilterValue, 37, 10, 0, // Skip to: 9689
/* 9679 */    MCD_OPC_CheckField, 6, 5, 0, 32, 1, // Skip to: 9973
/* 9685 */    MCD_OPC_Decode, 130, 4, 109, // Opcode: FDIVo
/* 9689 */    MCD_OPC_FilterValue, 40, 10, 0, // Skip to: 9703
/* 9693 */    MCD_OPC_CheckField, 6, 5, 0, 18, 1, // Skip to: 9973
/* 9699 */    MCD_OPC_Decode, 195, 4, 109, // Opcode: FSUB
/* 9703 */    MCD_OPC_FilterValue, 41, 10, 0, // Skip to: 9717
/* 9707 */    MCD_OPC_CheckField, 6, 5, 0, 4, 1, // Skip to: 9973
/* 9713 */    MCD_OPC_Decode, 198, 4, 109, // Opcode: FSUBo
/* 9717 */    MCD_OPC_FilterValue, 42, 10, 0, // Skip to: 9731
/* 9721 */    MCD_OPC_CheckField, 6, 5, 0, 246, 0, // Skip to: 9973
/* 9727 */    MCD_OPC_Decode, 224, 3, 109, // Opcode: FADD
/* 9731 */    MCD_OPC_FilterValue, 43, 10, 0, // Skip to: 9745
/* 9735 */    MCD_OPC_CheckField, 6, 5, 0, 232, 0, // Skip to: 9973
/* 9741 */    MCD_OPC_Decode, 227, 3, 109, // Opcode: FADDo
/* 9745 */    MCD_OPC_FilterValue, 44, 16, 0, // Skip to: 9765
/* 9749 */    MCD_OPC_CheckField, 16, 5, 0, 218, 0, // Skip to: 9973
/* 9755 */    MCD_OPC_CheckField, 6, 5, 0, 212, 0, // Skip to: 9973
/* 9761 */    MCD_OPC_Decode, 191, 4, 108, // Opcode: FSQRT
/* 9765 */    MCD_OPC_FilterValue, 45, 16, 0, // Skip to: 9785
/* 9769 */    MCD_OPC_CheckField, 16, 5, 0, 198, 0, // Skip to: 9973
/* 9775 */    MCD_OPC_CheckField, 6, 5, 0, 192, 0, // Skip to: 9973
/* 9781 */    MCD_OPC_Decode, 194, 4, 108, // Opcode: FSQRTo
/* 9785 */    MCD_OPC_FilterValue, 46, 4, 0, // Skip to: 9793
/* 9789 */    MCD_OPC_Decode, 189, 4, 110, // Opcode: FSELS
/* 9793 */    MCD_OPC_FilterValue, 47, 4, 0, // Skip to: 9801
/* 9797 */    MCD_OPC_Decode, 190, 4, 110, // Opcode: FSELSo
/* 9801 */    MCD_OPC_FilterValue, 48, 16, 0, // Skip to: 9821
/* 9805 */    MCD_OPC_CheckField, 16, 5, 0, 162, 0, // Skip to: 9973
/* 9811 */    MCD_OPC_CheckField, 6, 5, 0, 156, 0, // Skip to: 9973
/* 9817 */    MCD_OPC_Decode, 161, 4, 108, // Opcode: FRE
/* 9821 */    MCD_OPC_FilterValue, 49, 16, 0, // Skip to: 9841
/* 9825 */    MCD_OPC_CheckField, 16, 5, 0, 142, 0, // Skip to: 9973
/* 9831 */    MCD_OPC_CheckField, 6, 5, 0, 136, 0, // Skip to: 9973
/* 9837 */    MCD_OPC_Decode, 164, 4, 108, // Opcode: FREo
/* 9841 */    MCD_OPC_FilterValue, 50, 10, 0, // Skip to: 9855
/* 9845 */    MCD_OPC_CheckField, 11, 5, 0, 122, 0, // Skip to: 9973
/* 9851 */    MCD_OPC_Decode, 141, 4, 111, // Opcode: FMUL
/* 9855 */    MCD_OPC_FilterValue, 51, 10, 0, // Skip to: 9869
/* 9859 */    MCD_OPC_CheckField, 11, 5, 0, 108, 0, // Skip to: 9973
/* 9865 */    MCD_OPC_Decode, 144, 4, 111, // Opcode: FMULo
/* 9869 */    MCD_OPC_FilterValue, 52, 16, 0, // Skip to: 9889
/* 9873 */    MCD_OPC_CheckField, 16, 5, 0, 94, 0, // Skip to: 9973
/* 9879 */    MCD_OPC_CheckField, 6, 5, 0, 88, 0, // Skip to: 9973
/* 9885 */    MCD_OPC_Decode, 183, 4, 108, // Opcode: FRSQRTE
/* 9889 */    MCD_OPC_FilterValue, 53, 16, 0, // Skip to: 9909
/* 9893 */    MCD_OPC_CheckField, 16, 5, 0, 74, 0, // Skip to: 9973
/* 9899 */    MCD_OPC_CheckField, 6, 5, 0, 68, 0, // Skip to: 9973
/* 9905 */    MCD_OPC_Decode, 186, 4, 108, // Opcode: FRSQRTEo
/* 9909 */    MCD_OPC_FilterValue, 56, 4, 0, // Skip to: 9917
/* 9913 */    MCD_OPC_Decode, 137, 4, 112, // Opcode: FMSUB
/* 9917 */    MCD_OPC_FilterValue, 57, 4, 0, // Skip to: 9925
/* 9921 */    MCD_OPC_Decode, 140, 4, 112, // Opcode: FMSUBo
/* 9925 */    MCD_OPC_FilterValue, 58, 4, 0, // Skip to: 9933
/* 9929 */    MCD_OPC_Decode, 131, 4, 112, // Opcode: FMADD
/* 9933 */    MCD_OPC_FilterValue, 59, 4, 0, // Skip to: 9941
/* 9937 */    MCD_OPC_Decode, 134, 4, 112, // Opcode: FMADDo
/* 9941 */    MCD_OPC_FilterValue, 60, 4, 0, // Skip to: 9949
/* 9945 */    MCD_OPC_Decode, 157, 4, 112, // Opcode: FNMSUB
/* 9949 */    MCD_OPC_FilterValue, 61, 4, 0, // Skip to: 9957
/* 9953 */    MCD_OPC_Decode, 160, 4, 112, // Opcode: FNMSUBo
/* 9957 */    MCD_OPC_FilterValue, 62, 4, 0, // Skip to: 9965
/* 9961 */    MCD_OPC_Decode, 153, 4, 112, // Opcode: FNMADD
/* 9965 */    MCD_OPC_FilterValue, 63, 4, 0, // Skip to: 9973
/* 9969 */    MCD_OPC_Decode, 156, 4, 112, // Opcode: FNMADDo
/* 9973 */    MCD_OPC_Fail,
  0
};

static uint8_t DecoderTableQPX32[] = {
/* 0 */       MCD_OPC_ExtractField, 0, 6,  // Inst{5-0} ...
/* 3 */       MCD_OPC_FilterValue, 0, 59, 0, // Skip to: 66
/* 7 */       MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 10 */      MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 24
/* 14 */      MCD_OPC_CheckField, 26, 6, 4, 68, 7, // Skip to: 1880
/* 20 */      MCD_OPC_Decode, 163, 6, 113, // Opcode: QVFCMPEQb
/* 24 */      MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 38
/* 28 */      MCD_OPC_CheckField, 26, 6, 4, 54, 7, // Skip to: 1880
/* 34 */      MCD_OPC_Decode, 166, 6, 113, // Opcode: QVFCMPGTb
/* 38 */      MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 52
/* 42 */      MCD_OPC_CheckField, 26, 6, 4, 40, 7, // Skip to: 1880
/* 48 */      MCD_OPC_Decode, 233, 6, 113, // Opcode: QVFTSTNANb
/* 52 */      MCD_OPC_FilterValue, 3, 32, 7, // Skip to: 1880
/* 56 */      MCD_OPC_CheckField, 26, 6, 4, 26, 7, // Skip to: 1880
/* 62 */      MCD_OPC_Decode, 169, 6, 113, // Opcode: QVFCMPLTb
/* 66 */      MCD_OPC_FilterValue, 2, 19, 0, // Skip to: 89
/* 70 */      MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 73 */      MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 81
/* 77 */      MCD_OPC_Decode, 242, 6, 114, // Opcode: QVFXXMADDS
/* 81 */      MCD_OPC_FilterValue, 4, 3, 7, // Skip to: 1880
/* 85 */      MCD_OPC_Decode, 241, 6, 114, // Opcode: QVFXXMADD
/* 89 */      MCD_OPC_FilterValue, 6, 19, 0, // Skip to: 112
/* 93 */      MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 96 */      MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 104
/* 100 */     MCD_OPC_Decode, 240, 6, 114, // Opcode: QVFXXCPNMADDS
/* 104 */     MCD_OPC_FilterValue, 4, 236, 6, // Skip to: 1880
/* 108 */     MCD_OPC_Decode, 239, 6, 114, // Opcode: QVFXXCPNMADD
/* 112 */     MCD_OPC_FilterValue, 8, 16, 0, // Skip to: 132
/* 116 */     MCD_OPC_CheckField, 26, 6, 4, 222, 6, // Skip to: 1880
/* 122 */     MCD_OPC_CheckField, 6, 1, 0, 216, 6, // Skip to: 1880
/* 128 */     MCD_OPC_Decode, 183, 6, 115, // Opcode: QVFLOGICALb
/* 132 */     MCD_OPC_FilterValue, 10, 180, 0, // Skip to: 316
/* 136 */     MCD_OPC_ExtractField, 6, 3,  // Inst{8-6} ...
/* 139 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 153
/* 143 */     MCD_OPC_CheckField, 26, 6, 4, 195, 6, // Skip to: 1880
/* 149 */     MCD_OPC_Decode, 146, 6, 116, // Opcode: QVALIGNI
/* 153 */     MCD_OPC_FilterValue, 1, 16, 0, // Skip to: 173
/* 157 */     MCD_OPC_CheckField, 26, 6, 4, 181, 6, // Skip to: 1880
/* 163 */     MCD_OPC_CheckField, 11, 5, 0, 175, 6, // Skip to: 1880
/* 169 */     MCD_OPC_Decode, 149, 6, 117, // Opcode: QVESPLATI
/* 173 */     MCD_OPC_FilterValue, 4, 34, 0, // Skip to: 211
/* 177 */     MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 180 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 188
/* 184 */     MCD_OPC_Decode, 245, 6, 118, // Opcode: QVGPCI
/* 188 */     MCD_OPC_FilterValue, 31, 152, 6, // Skip to: 1880
/* 192 */     MCD_OPC_ExtractField, 9, 2,  // Inst{10-9} ...
/* 195 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 203
/* 199 */     MCD_OPC_Decode, 161, 7, 119, // Opcode: QVSTFCSXI
/* 203 */     MCD_OPC_FilterValue, 2, 137, 6, // Skip to: 1880
/* 207 */     MCD_OPC_Decode, 182, 7, 119, // Opcode: QVSTFSXI
/* 211 */     MCD_OPC_FilterValue, 5, 31, 0, // Skip to: 246
/* 215 */     MCD_OPC_ExtractField, 9, 2,  // Inst{10-9} ...
/* 218 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 232
/* 222 */     MCD_OPC_CheckField, 26, 6, 31, 116, 6, // Skip to: 1880
/* 228 */     MCD_OPC_Decode, 157, 7, 119, // Opcode: QVSTFCSUXI
/* 232 */     MCD_OPC_FilterValue, 2, 108, 6, // Skip to: 1880
/* 236 */     MCD_OPC_CheckField, 26, 6, 31, 102, 6, // Skip to: 1880
/* 242 */     MCD_OPC_Decode, 177, 7, 119, // Opcode: QVSTFSUXI
/* 246 */     MCD_OPC_FilterValue, 6, 31, 0, // Skip to: 281
/* 250 */     MCD_OPC_ExtractField, 9, 2,  // Inst{10-9} ...
/* 253 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 267
/* 257 */     MCD_OPC_CheckField, 26, 6, 31, 81, 6, // Skip to: 1880
/* 263 */     MCD_OPC_Decode, 153, 7, 119, // Opcode: QVSTFCDXI
/* 267 */     MCD_OPC_FilterValue, 2, 73, 6, // Skip to: 1880
/* 271 */     MCD_OPC_CheckField, 26, 6, 31, 67, 6, // Skip to: 1880
/* 277 */     MCD_OPC_Decode, 170, 7, 119, // Opcode: QVSTFDXI
/* 281 */     MCD_OPC_FilterValue, 7, 59, 6, // Skip to: 1880
/* 285 */     MCD_OPC_ExtractField, 9, 2,  // Inst{10-9} ...
/* 288 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 302
/* 292 */     MCD_OPC_CheckField, 26, 6, 31, 46, 6, // Skip to: 1880
/* 298 */     MCD_OPC_Decode, 149, 7, 119, // Opcode: QVSTFCDUXI
/* 302 */     MCD_OPC_FilterValue, 2, 38, 6, // Skip to: 1880
/* 306 */     MCD_OPC_CheckField, 26, 6, 31, 32, 6, // Skip to: 1880
/* 312 */     MCD_OPC_Decode, 166, 7, 119, // Opcode: QVSTFDUXI
/* 316 */     MCD_OPC_FilterValue, 11, 115, 0, // Skip to: 435
/* 320 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 323 */     MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 337
/* 327 */     MCD_OPC_CheckField, 26, 6, 31, 11, 6, // Skip to: 1880
/* 333 */     MCD_OPC_Decode, 162, 7, 119, // Opcode: QVSTFCSXIA
/* 337 */     MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 351
/* 341 */     MCD_OPC_CheckField, 26, 6, 31, 253, 5, // Skip to: 1880
/* 347 */     MCD_OPC_Decode, 158, 7, 119, // Opcode: QVSTFCSUXIA
/* 351 */     MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 365
/* 355 */     MCD_OPC_CheckField, 26, 6, 31, 239, 5, // Skip to: 1880
/* 361 */     MCD_OPC_Decode, 154, 7, 119, // Opcode: QVSTFCDXIA
/* 365 */     MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 379
/* 369 */     MCD_OPC_CheckField, 26, 6, 31, 225, 5, // Skip to: 1880
/* 375 */     MCD_OPC_Decode, 150, 7, 119, // Opcode: QVSTFCDUXIA
/* 379 */     MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 393
/* 383 */     MCD_OPC_CheckField, 26, 6, 31, 211, 5, // Skip to: 1880
/* 389 */     MCD_OPC_Decode, 183, 7, 119, // Opcode: QVSTFSXIA
/* 393 */     MCD_OPC_FilterValue, 21, 10, 0, // Skip to: 407
/* 397 */     MCD_OPC_CheckField, 26, 6, 31, 197, 5, // Skip to: 1880
/* 403 */     MCD_OPC_Decode, 178, 7, 119, // Opcode: QVSTFSUXIA
/* 407 */     MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 421
/* 411 */     MCD_OPC_CheckField, 26, 6, 31, 183, 5, // Skip to: 1880
/* 417 */     MCD_OPC_Decode, 171, 7, 119, // Opcode: QVSTFDXIA
/* 421 */     MCD_OPC_FilterValue, 23, 175, 5, // Skip to: 1880
/* 425 */     MCD_OPC_CheckField, 26, 6, 31, 169, 5, // Skip to: 1880
/* 431 */     MCD_OPC_Decode, 167, 7, 119, // Opcode: QVSTFDUXIA
/* 435 */     MCD_OPC_FilterValue, 12, 50, 0, // Skip to: 489
/* 439 */     MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 442 */     MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 450
/* 446 */     MCD_OPC_Decode, 207, 6, 114, // Opcode: QVFPERM
/* 450 */     MCD_OPC_FilterValue, 31, 146, 5, // Skip to: 1880
/* 454 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 457 */     MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 465
/* 461 */     MCD_OPC_Decode, 146, 7, 119, // Opcode: QVLPCRSX
/* 465 */     MCD_OPC_FilterValue, 2, 4, 0, // Skip to: 473
/* 469 */     MCD_OPC_Decode, 145, 7, 119, // Opcode: QVLPCRDX
/* 473 */     MCD_OPC_FilterValue, 16, 4, 0, // Skip to: 481
/* 477 */     MCD_OPC_Decode, 143, 7, 119, // Opcode: QVLPCLSX
/* 481 */     MCD_OPC_FilterValue, 18, 115, 5, // Skip to: 1880
/* 485 */     MCD_OPC_Decode, 142, 7, 119, // Opcode: QVLPCLDX
/* 489 */     MCD_OPC_FilterValue, 14, 13, 1, // Skip to: 762
/* 493 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 496 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 510
/* 500 */     MCD_OPC_CheckField, 26, 6, 31, 94, 5, // Skip to: 1880
/* 506 */     MCD_OPC_Decode, 252, 6, 119, // Opcode: QVLFCSX
/* 510 */     MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 524
/* 514 */     MCD_OPC_CheckField, 26, 6, 31, 80, 5, // Skip to: 1880
/* 520 */     MCD_OPC_Decode, 250, 6, 119, // Opcode: QVLFCSUX
/* 524 */     MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 538
/* 528 */     MCD_OPC_CheckField, 26, 6, 31, 66, 5, // Skip to: 1880
/* 534 */     MCD_OPC_Decode, 248, 6, 119, // Opcode: QVLFCDX
/* 538 */     MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 552
/* 542 */     MCD_OPC_CheckField, 26, 6, 31, 52, 5, // Skip to: 1880
/* 548 */     MCD_OPC_Decode, 246, 6, 119, // Opcode: QVLFCDUX
/* 552 */     MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 566
/* 556 */     MCD_OPC_CheckField, 26, 6, 31, 38, 5, // Skip to: 1880
/* 562 */     MCD_OPC_Decode, 159, 7, 119, // Opcode: QVSTFCSX
/* 566 */     MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 580
/* 570 */     MCD_OPC_CheckField, 26, 6, 31, 24, 5, // Skip to: 1880
/* 576 */     MCD_OPC_Decode, 155, 7, 119, // Opcode: QVSTFCSUX
/* 580 */     MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 594
/* 584 */     MCD_OPC_CheckField, 26, 6, 31, 10, 5, // Skip to: 1880
/* 590 */     MCD_OPC_Decode, 151, 7, 119, // Opcode: QVSTFCDX
/* 594 */     MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 608
/* 598 */     MCD_OPC_CheckField, 26, 6, 31, 252, 4, // Skip to: 1880
/* 604 */     MCD_OPC_Decode, 147, 7, 119, // Opcode: QVSTFCDUX
/* 608 */     MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 622
/* 612 */     MCD_OPC_CheckField, 26, 6, 31, 238, 4, // Skip to: 1880
/* 618 */     MCD_OPC_Decode, 138, 7, 119, // Opcode: QVLFSX
/* 622 */     MCD_OPC_FilterValue, 17, 10, 0, // Skip to: 636
/* 626 */     MCD_OPC_CheckField, 26, 6, 31, 224, 4, // Skip to: 1880
/* 632 */     MCD_OPC_Decode, 136, 7, 120, // Opcode: QVLFSUX
/* 636 */     MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 650
/* 640 */     MCD_OPC_CheckField, 26, 6, 31, 210, 4, // Skip to: 1880
/* 646 */     MCD_OPC_Decode, 129, 7, 119, // Opcode: QVLFDX
/* 650 */     MCD_OPC_FilterValue, 19, 10, 0, // Skip to: 664
/* 654 */     MCD_OPC_CheckField, 26, 6, 31, 196, 4, // Skip to: 1880
/* 660 */     MCD_OPC_Decode, 255, 6, 121, // Opcode: QVLFDUX
/* 664 */     MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 678
/* 668 */     MCD_OPC_CheckField, 26, 6, 31, 182, 4, // Skip to: 1880
/* 674 */     MCD_OPC_Decode, 180, 7, 119, // Opcode: QVSTFSX
/* 678 */     MCD_OPC_FilterValue, 21, 10, 0, // Skip to: 692
/* 682 */     MCD_OPC_CheckField, 26, 6, 31, 168, 4, // Skip to: 1880
/* 688 */     MCD_OPC_Decode, 175, 7, 122, // Opcode: QVSTFSUX
/* 692 */     MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 706
/* 696 */     MCD_OPC_CheckField, 26, 6, 31, 154, 4, // Skip to: 1880
/* 702 */     MCD_OPC_Decode, 168, 7, 119, // Opcode: QVSTFDX
/* 706 */     MCD_OPC_FilterValue, 23, 10, 0, // Skip to: 720
/* 710 */     MCD_OPC_CheckField, 26, 6, 31, 140, 4, // Skip to: 1880
/* 716 */     MCD_OPC_Decode, 164, 7, 123, // Opcode: QVSTFDUX
/* 720 */     MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 734
/* 724 */     MCD_OPC_CheckField, 26, 6, 31, 126, 4, // Skip to: 1880
/* 730 */     MCD_OPC_Decode, 134, 7, 119, // Opcode: QVLFIWZX
/* 734 */     MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 748
/* 738 */     MCD_OPC_CheckField, 26, 6, 31, 112, 4, // Skip to: 1880
/* 744 */     MCD_OPC_Decode, 132, 7, 119, // Opcode: QVLFIWAX
/* 748 */     MCD_OPC_FilterValue, 30, 104, 4, // Skip to: 1880
/* 752 */     MCD_OPC_CheckField, 26, 6, 31, 98, 4, // Skip to: 1880
/* 758 */     MCD_OPC_Decode, 173, 7, 119, // Opcode: QVSTFIWX
/* 762 */     MCD_OPC_FilterValue, 15, 13, 1, // Skip to: 1035
/* 766 */     MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 769 */     MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 783
/* 773 */     MCD_OPC_CheckField, 26, 6, 31, 77, 4, // Skip to: 1880
/* 779 */     MCD_OPC_Decode, 253, 6, 119, // Opcode: QVLFCSXA
/* 783 */     MCD_OPC_FilterValue, 1, 10, 0, // Skip to: 797
/* 787 */     MCD_OPC_CheckField, 26, 6, 31, 63, 4, // Skip to: 1880
/* 793 */     MCD_OPC_Decode, 251, 6, 119, // Opcode: QVLFCSUXA
/* 797 */     MCD_OPC_FilterValue, 2, 10, 0, // Skip to: 811
/* 801 */     MCD_OPC_CheckField, 26, 6, 31, 49, 4, // Skip to: 1880
/* 807 */     MCD_OPC_Decode, 249, 6, 119, // Opcode: QVLFCDXA
/* 811 */     MCD_OPC_FilterValue, 3, 10, 0, // Skip to: 825
/* 815 */     MCD_OPC_CheckField, 26, 6, 31, 35, 4, // Skip to: 1880
/* 821 */     MCD_OPC_Decode, 247, 6, 119, // Opcode: QVLFCDUXA
/* 825 */     MCD_OPC_FilterValue, 4, 10, 0, // Skip to: 839
/* 829 */     MCD_OPC_CheckField, 26, 6, 31, 21, 4, // Skip to: 1880
/* 835 */     MCD_OPC_Decode, 160, 7, 119, // Opcode: QVSTFCSXA
/* 839 */     MCD_OPC_FilterValue, 5, 10, 0, // Skip to: 853
/* 843 */     MCD_OPC_CheckField, 26, 6, 31, 7, 4, // Skip to: 1880
/* 849 */     MCD_OPC_Decode, 156, 7, 119, // Opcode: QVSTFCSUXA
/* 853 */     MCD_OPC_FilterValue, 6, 10, 0, // Skip to: 867
/* 857 */     MCD_OPC_CheckField, 26, 6, 31, 249, 3, // Skip to: 1880
/* 863 */     MCD_OPC_Decode, 152, 7, 119, // Opcode: QVSTFCDXA
/* 867 */     MCD_OPC_FilterValue, 7, 10, 0, // Skip to: 881
/* 871 */     MCD_OPC_CheckField, 26, 6, 31, 235, 3, // Skip to: 1880
/* 877 */     MCD_OPC_Decode, 148, 7, 119, // Opcode: QVSTFCDUXA
/* 881 */     MCD_OPC_FilterValue, 16, 10, 0, // Skip to: 895
/* 885 */     MCD_OPC_CheckField, 26, 6, 31, 221, 3, // Skip to: 1880
/* 891 */     MCD_OPC_Decode, 139, 7, 119, // Opcode: QVLFSXA
/* 895 */     MCD_OPC_FilterValue, 17, 10, 0, // Skip to: 909
/* 899 */     MCD_OPC_CheckField, 26, 6, 31, 207, 3, // Skip to: 1880
/* 905 */     MCD_OPC_Decode, 137, 7, 119, // Opcode: QVLFSUXA
/* 909 */     MCD_OPC_FilterValue, 18, 10, 0, // Skip to: 923
/* 913 */     MCD_OPC_CheckField, 26, 6, 31, 193, 3, // Skip to: 1880
/* 919 */     MCD_OPC_Decode, 130, 7, 119, // Opcode: QVLFDXA
/* 923 */     MCD_OPC_FilterValue, 19, 10, 0, // Skip to: 937
/* 927 */     MCD_OPC_CheckField, 26, 6, 31, 179, 3, // Skip to: 1880
/* 933 */     MCD_OPC_Decode, 128, 7, 119, // Opcode: QVLFDUXA
/* 937 */     MCD_OPC_FilterValue, 20, 10, 0, // Skip to: 951
/* 941 */     MCD_OPC_CheckField, 26, 6, 31, 165, 3, // Skip to: 1880
/* 947 */     MCD_OPC_Decode, 181, 7, 119, // Opcode: QVSTFSXA
/* 951 */     MCD_OPC_FilterValue, 21, 10, 0, // Skip to: 965
/* 955 */     MCD_OPC_CheckField, 26, 6, 31, 151, 3, // Skip to: 1880
/* 961 */     MCD_OPC_Decode, 176, 7, 119, // Opcode: QVSTFSUXA
/* 965 */     MCD_OPC_FilterValue, 22, 10, 0, // Skip to: 979
/* 969 */     MCD_OPC_CheckField, 26, 6, 31, 137, 3, // Skip to: 1880
/* 975 */     MCD_OPC_Decode, 169, 7, 119, // Opcode: QVSTFDXA
/* 979 */     MCD_OPC_FilterValue, 23, 10, 0, // Skip to: 993
/* 983 */     MCD_OPC_CheckField, 26, 6, 31, 123, 3, // Skip to: 1880
/* 989 */     MCD_OPC_Decode, 165, 7, 119, // Opcode: QVSTFDUXA
/* 993 */     MCD_OPC_FilterValue, 26, 10, 0, // Skip to: 1007
/* 997 */     MCD_OPC_CheckField, 26, 6, 31, 109, 3, // Skip to: 1880
/* 1003 */    MCD_OPC_Decode, 135, 7, 119, // Opcode: QVLFIWZXA
/* 1007 */    MCD_OPC_FilterValue, 27, 10, 0, // Skip to: 1021
/* 1011 */    MCD_OPC_CheckField, 26, 6, 31, 95, 3, // Skip to: 1880
/* 1017 */    MCD_OPC_Decode, 133, 7, 119, // Opcode: QVLFIWAXA
/* 1021 */    MCD_OPC_FilterValue, 30, 87, 3, // Skip to: 1880
/* 1025 */    MCD_OPC_CheckField, 26, 6, 31, 81, 3, // Skip to: 1880
/* 1031 */    MCD_OPC_Decode, 174, 7, 119, // Opcode: QVSTFIWXA
/* 1035 */    MCD_OPC_FilterValue, 16, 177, 0, // Skip to: 1216
/* 1039 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1042 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1056
/* 1046 */    MCD_OPC_CheckField, 26, 6, 4, 60, 3, // Skip to: 1880
/* 1052 */    MCD_OPC_Decode, 171, 6, 124, // Opcode: QVFCPSGN
/* 1056 */    MCD_OPC_FilterValue, 1, 16, 0, // Skip to: 1076
/* 1060 */    MCD_OPC_CheckField, 26, 6, 4, 46, 3, // Skip to: 1880
/* 1066 */    MCD_OPC_CheckField, 16, 5, 0, 40, 3, // Skip to: 1880
/* 1072 */    MCD_OPC_Decode, 199, 6, 125, // Opcode: QVFNEG
/* 1076 */    MCD_OPC_FilterValue, 2, 16, 0, // Skip to: 1096
/* 1080 */    MCD_OPC_CheckField, 26, 6, 4, 26, 3, // Skip to: 1880
/* 1086 */    MCD_OPC_CheckField, 16, 5, 0, 20, 3, // Skip to: 1880
/* 1092 */    MCD_OPC_Decode, 188, 6, 125, // Opcode: QVFMR
/* 1096 */    MCD_OPC_FilterValue, 4, 16, 0, // Skip to: 1116
/* 1100 */    MCD_OPC_CheckField, 26, 6, 4, 6, 3, // Skip to: 1880
/* 1106 */    MCD_OPC_CheckField, 16, 5, 0, 0, 3, // Skip to: 1880
/* 1112 */    MCD_OPC_Decode, 197, 6, 125, // Opcode: QVFNABS
/* 1116 */    MCD_OPC_FilterValue, 8, 16, 0, // Skip to: 1136
/* 1120 */    MCD_OPC_CheckField, 26, 6, 4, 242, 2, // Skip to: 1880
/* 1126 */    MCD_OPC_CheckField, 16, 5, 0, 236, 2, // Skip to: 1880
/* 1132 */    MCD_OPC_Decode, 152, 6, 125, // Opcode: QVFABS
/* 1136 */    MCD_OPC_FilterValue, 12, 16, 0, // Skip to: 1156
/* 1140 */    MCD_OPC_CheckField, 26, 6, 4, 222, 2, // Skip to: 1880
/* 1146 */    MCD_OPC_CheckField, 16, 5, 0, 216, 2, // Skip to: 1880
/* 1152 */    MCD_OPC_Decode, 214, 6, 125, // Opcode: QVFRIN
/* 1156 */    MCD_OPC_FilterValue, 13, 16, 0, // Skip to: 1176
/* 1160 */    MCD_OPC_CheckField, 26, 6, 4, 202, 2, // Skip to: 1880
/* 1166 */    MCD_OPC_CheckField, 16, 5, 0, 196, 2, // Skip to: 1880
/* 1172 */    MCD_OPC_Decode, 218, 6, 125, // Opcode: QVFRIZ
/* 1176 */    MCD_OPC_FilterValue, 14, 16, 0, // Skip to: 1196
/* 1180 */    MCD_OPC_CheckField, 26, 6, 4, 182, 2, // Skip to: 1880
/* 1186 */    MCD_OPC_CheckField, 16, 5, 0, 176, 2, // Skip to: 1880
/* 1192 */    MCD_OPC_Decode, 216, 6, 125, // Opcode: QVFRIP
/* 1196 */    MCD_OPC_FilterValue, 15, 168, 2, // Skip to: 1880
/* 1200 */    MCD_OPC_CheckField, 26, 6, 4, 162, 2, // Skip to: 1880
/* 1206 */    MCD_OPC_CheckField, 16, 5, 0, 156, 2, // Skip to: 1880
/* 1212 */    MCD_OPC_Decode, 212, 6, 125, // Opcode: QVFRIM
/* 1216 */    MCD_OPC_FilterValue, 18, 19, 0, // Skip to: 1239
/* 1220 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1223 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1231
/* 1227 */    MCD_OPC_Decode, 236, 6, 114, // Opcode: QVFXMADDS
/* 1231 */    MCD_OPC_FilterValue, 4, 133, 2, // Skip to: 1880
/* 1235 */    MCD_OPC_Decode, 235, 6, 114, // Opcode: QVFXMADD
/* 1239 */    MCD_OPC_FilterValue, 22, 19, 0, // Skip to: 1262
/* 1243 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1246 */    MCD_OPC_FilterValue, 0, 4, 0, // Skip to: 1254
/* 1250 */    MCD_OPC_Decode, 244, 6, 114, // Opcode: QVFXXNPMADDS
/* 1254 */    MCD_OPC_FilterValue, 4, 110, 2, // Skip to: 1880
/* 1258 */    MCD_OPC_Decode, 243, 6, 114, // Opcode: QVFXXNPMADD
/* 1262 */    MCD_OPC_FilterValue, 24, 22, 0, // Skip to: 1288
/* 1266 */    MCD_OPC_CheckField, 26, 6, 4, 96, 2, // Skip to: 1880
/* 1272 */    MCD_OPC_CheckField, 16, 5, 0, 90, 2, // Skip to: 1880
/* 1278 */    MCD_OPC_CheckField, 6, 5, 0, 84, 2, // Skip to: 1880
/* 1284 */    MCD_OPC_Decode, 221, 6, 126, // Opcode: QVFRSPs
/* 1288 */    MCD_OPC_FilterValue, 28, 153, 0, // Skip to: 1445
/* 1292 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1295 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 1315
/* 1299 */    MCD_OPC_CheckField, 26, 6, 4, 63, 2, // Skip to: 1880
/* 1305 */    MCD_OPC_CheckField, 16, 5, 0, 57, 2, // Skip to: 1880
/* 1311 */    MCD_OPC_Decode, 178, 6, 125, // Opcode: QVFCTIW
/* 1315 */    MCD_OPC_FilterValue, 4, 16, 0, // Skip to: 1335
/* 1319 */    MCD_OPC_CheckField, 26, 6, 4, 43, 2, // Skip to: 1880
/* 1325 */    MCD_OPC_CheckField, 16, 5, 0, 37, 2, // Skip to: 1880
/* 1331 */    MCD_OPC_Decode, 179, 6, 125, // Opcode: QVFCTIWU
/* 1335 */    MCD_OPC_FilterValue, 25, 16, 0, // Skip to: 1355
/* 1339 */    MCD_OPC_CheckField, 26, 6, 4, 23, 2, // Skip to: 1880
/* 1345 */    MCD_OPC_CheckField, 16, 5, 0, 17, 2, // Skip to: 1880
/* 1351 */    MCD_OPC_Decode, 173, 6, 125, // Opcode: QVFCTID
/* 1355 */    MCD_OPC_FilterValue, 26, 31, 0, // Skip to: 1390
/* 1359 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1362 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1376
/* 1366 */    MCD_OPC_CheckField, 16, 5, 0, 252, 1, // Skip to: 1880
/* 1372 */    MCD_OPC_Decode, 158, 6, 125, // Opcode: QVFCFIDS
/* 1376 */    MCD_OPC_FilterValue, 4, 244, 1, // Skip to: 1880
/* 1380 */    MCD_OPC_CheckField, 16, 5, 0, 238, 1, // Skip to: 1880
/* 1386 */    MCD_OPC_Decode, 157, 6, 125, // Opcode: QVFCFID
/* 1390 */    MCD_OPC_FilterValue, 29, 16, 0, // Skip to: 1410
/* 1394 */    MCD_OPC_CheckField, 26, 6, 4, 224, 1, // Skip to: 1880
/* 1400 */    MCD_OPC_CheckField, 16, 5, 0, 218, 1, // Skip to: 1880
/* 1406 */    MCD_OPC_Decode, 174, 6, 125, // Opcode: QVFCTIDU
/* 1410 */    MCD_OPC_FilterValue, 30, 210, 1, // Skip to: 1880
/* 1414 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1417 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1431
/* 1421 */    MCD_OPC_CheckField, 16, 5, 0, 197, 1, // Skip to: 1880
/* 1427 */    MCD_OPC_Decode, 160, 6, 125, // Opcode: QVFCFIDUS
/* 1431 */    MCD_OPC_FilterValue, 4, 189, 1, // Skip to: 1880
/* 1435 */    MCD_OPC_CheckField, 16, 5, 0, 183, 1, // Skip to: 1880
/* 1441 */    MCD_OPC_Decode, 159, 6, 125, // Opcode: QVFCFIDU
/* 1445 */    MCD_OPC_FilterValue, 30, 83, 0, // Skip to: 1532
/* 1449 */    MCD_OPC_ExtractField, 6, 5,  // Inst{10-6} ...
/* 1452 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 1472
/* 1456 */    MCD_OPC_CheckField, 26, 6, 4, 162, 1, // Skip to: 1880
/* 1462 */    MCD_OPC_CheckField, 16, 5, 0, 156, 1, // Skip to: 1880
/* 1468 */    MCD_OPC_Decode, 181, 6, 125, // Opcode: QVFCTIWZ
/* 1472 */    MCD_OPC_FilterValue, 4, 16, 0, // Skip to: 1492
/* 1476 */    MCD_OPC_CheckField, 26, 6, 4, 142, 1, // Skip to: 1880
/* 1482 */    MCD_OPC_CheckField, 16, 5, 0, 136, 1, // Skip to: 1880
/* 1488 */    MCD_OPC_Decode, 180, 6, 125, // Opcode: QVFCTIWUZ
/* 1492 */    MCD_OPC_FilterValue, 25, 16, 0, // Skip to: 1512
/* 1496 */    MCD_OPC_CheckField, 26, 6, 4, 122, 1, // Skip to: 1880
/* 1502 */    MCD_OPC_CheckField, 16, 5, 0, 116, 1, // Skip to: 1880
/* 1508 */    MCD_OPC_Decode, 176, 6, 125, // Opcode: QVFCTIDZ
/* 1512 */    MCD_OPC_FilterValue, 29, 108, 1, // Skip to: 1880
/* 1516 */    MCD_OPC_CheckField, 26, 6, 4, 102, 1, // Skip to: 1880
/* 1522 */    MCD_OPC_CheckField, 16, 5, 0, 96, 1, // Skip to: 1880
/* 1528 */    MCD_OPC_Decode, 175, 6, 125, // Opcode: QVFCTIDUZ
/* 1532 */    MCD_OPC_FilterValue, 34, 31, 0, // Skip to: 1567
/* 1536 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1539 */    MCD_OPC_FilterValue, 0, 10, 0, // Skip to: 1553
/* 1543 */    MCD_OPC_CheckField, 11, 5, 0, 75, 1, // Skip to: 1880
/* 1549 */    MCD_OPC_Decode, 238, 6, 127, // Opcode: QVFXMULS
/* 1553 */    MCD_OPC_FilterValue, 4, 67, 1, // Skip to: 1880
/* 1557 */    MCD_OPC_CheckField, 11, 5, 0, 61, 1, // Skip to: 1880
/* 1563 */    MCD_OPC_Decode, 237, 6, 127, // Opcode: QVFXMUL
/* 1567 */    MCD_OPC_FilterValue, 40, 32, 0, // Skip to: 1603
/* 1571 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1574 */    MCD_OPC_FilterValue, 0, 11, 0, // Skip to: 1589
/* 1578 */    MCD_OPC_CheckField, 6, 5, 0, 40, 1, // Skip to: 1880
/* 1584 */    MCD_OPC_Decode, 231, 6, 128, 1, // Opcode: QVFSUBSs
/* 1589 */    MCD_OPC_FilterValue, 4, 31, 1, // Skip to: 1880
/* 1593 */    MCD_OPC_CheckField, 6, 5, 0, 25, 1, // Skip to: 1880
/* 1599 */    MCD_OPC_Decode, 229, 6, 124, // Opcode: QVFSUB
/* 1603 */    MCD_OPC_FilterValue, 42, 32, 0, // Skip to: 1639
/* 1607 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1610 */    MCD_OPC_FilterValue, 0, 11, 0, // Skip to: 1625
/* 1614 */    MCD_OPC_CheckField, 6, 5, 0, 4, 1, // Skip to: 1880
/* 1620 */    MCD_OPC_Decode, 156, 6, 128, 1, // Opcode: QVFADDSs
/* 1625 */    MCD_OPC_FilterValue, 4, 251, 0, // Skip to: 1880
/* 1629 */    MCD_OPC_CheckField, 6, 5, 0, 245, 0, // Skip to: 1880
/* 1635 */    MCD_OPC_Decode, 154, 6, 124, // Opcode: QVFADD
/* 1639 */    MCD_OPC_FilterValue, 46, 11, 0, // Skip to: 1654
/* 1643 */    MCD_OPC_CheckField, 26, 6, 4, 231, 0, // Skip to: 1880
/* 1649 */    MCD_OPC_Decode, 226, 6, 129, 1, // Opcode: QVFSELb
/* 1654 */    MCD_OPC_FilterValue, 48, 43, 0, // Skip to: 1701
/* 1658 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1661 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 1681
/* 1665 */    MCD_OPC_CheckField, 16, 5, 0, 209, 0, // Skip to: 1880
/* 1671 */    MCD_OPC_CheckField, 6, 5, 0, 203, 0, // Skip to: 1880
/* 1677 */    MCD_OPC_Decode, 210, 6, 125, // Opcode: QVFRES
/* 1681 */    MCD_OPC_FilterValue, 4, 195, 0, // Skip to: 1880
/* 1685 */    MCD_OPC_CheckField, 16, 5, 0, 189, 0, // Skip to: 1880
/* 1691 */    MCD_OPC_CheckField, 6, 5, 0, 183, 0, // Skip to: 1880
/* 1697 */    MCD_OPC_Decode, 209, 6, 125, // Opcode: QVFRE
/* 1701 */    MCD_OPC_FilterValue, 50, 32, 0, // Skip to: 1737
/* 1705 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1708 */    MCD_OPC_FilterValue, 0, 11, 0, // Skip to: 1723
/* 1712 */    MCD_OPC_CheckField, 11, 5, 0, 162, 0, // Skip to: 1880
/* 1718 */    MCD_OPC_Decode, 196, 6, 130, 1, // Opcode: QVFMULSs
/* 1723 */    MCD_OPC_FilterValue, 4, 153, 0, // Skip to: 1880
/* 1727 */    MCD_OPC_CheckField, 11, 5, 0, 147, 0, // Skip to: 1880
/* 1733 */    MCD_OPC_Decode, 194, 6, 127, // Opcode: QVFMUL
/* 1737 */    MCD_OPC_FilterValue, 52, 43, 0, // Skip to: 1784
/* 1741 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1744 */    MCD_OPC_FilterValue, 0, 16, 0, // Skip to: 1764
/* 1748 */    MCD_OPC_CheckField, 16, 5, 0, 126, 0, // Skip to: 1880
/* 1754 */    MCD_OPC_CheckField, 6, 5, 0, 120, 0, // Skip to: 1880
/* 1760 */    MCD_OPC_Decode, 223, 6, 125, // Opcode: QVFRSQRTES
/* 1764 */    MCD_OPC_FilterValue, 4, 112, 0, // Skip to: 1880
/* 1768 */    MCD_OPC_CheckField, 16, 5, 0, 106, 0, // Skip to: 1880
/* 1774 */    MCD_OPC_CheckField, 6, 5, 0, 100, 0, // Skip to: 1880
/* 1780 */    MCD_OPC_Decode, 222, 6, 125, // Opcode: QVFRSQRTE
/* 1784 */    MCD_OPC_FilterValue, 56, 20, 0, // Skip to: 1808
/* 1788 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1791 */    MCD_OPC_FilterValue, 0, 5, 0, // Skip to: 1800
/* 1795 */    MCD_OPC_Decode, 193, 6, 131, 1, // Opcode: QVFMSUBSs
/* 1800 */    MCD_OPC_FilterValue, 4, 76, 0, // Skip to: 1880
/* 1804 */    MCD_OPC_Decode, 191, 6, 114, // Opcode: QVFMSUB
/* 1808 */    MCD_OPC_FilterValue, 58, 20, 0, // Skip to: 1832
/* 1812 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1815 */    MCD_OPC_FilterValue, 0, 5, 0, // Skip to: 1824
/* 1819 */    MCD_OPC_Decode, 187, 6, 131, 1, // Opcode: QVFMADDSs
/* 1824 */    MCD_OPC_FilterValue, 4, 52, 0, // Skip to: 1880
/* 1828 */    MCD_OPC_Decode, 185, 6, 114, // Opcode: QVFMADD
/* 1832 */    MCD_OPC_FilterValue, 60, 20, 0, // Skip to: 1856
/* 1836 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1839 */    MCD_OPC_FilterValue, 0, 5, 0, // Skip to: 1848
/* 1843 */    MCD_OPC_Decode, 206, 6, 131, 1, // Opcode: QVFNMSUBSs
/* 1848 */    MCD_OPC_FilterValue, 4, 28, 0, // Skip to: 1880
/* 1852 */    MCD_OPC_Decode, 204, 6, 114, // Opcode: QVFNMSUB
/* 1856 */    MCD_OPC_FilterValue, 62, 20, 0, // Skip to: 1880
/* 1860 */    MCD_OPC_ExtractField, 26, 6,  // Inst{31-26} ...
/* 1863 */    MCD_OPC_FilterValue, 0, 5, 0, // Skip to: 1872
/* 1867 */    MCD_OPC_Decode, 203, 6, 131, 1, // Opcode: QVFNMADDSs
/* 1872 */    MCD_OPC_FilterValue, 4, 4, 0, // Skip to: 1880
/* 1876 */    MCD_OPC_Decode, 201, 6, 114, // Opcode: QVFNMADD
/* 1880 */    MCD_OPC_Fail,
  0
};
static bool checkDecoderPredicate(unsigned Idx, uint64_t Bits)
{
  //llvm_unreachable("Invalid index!");
  return true;
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
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 2: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 3: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 4: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 5: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 6: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 7: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 8: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 9: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 10: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 4); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 11: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 12: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 13: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 14: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 15: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 16: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 17: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 18: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRC_NOR0RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeSImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 19: \
    tmp = fieldname(insn, 2, 14); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 20: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 2, 14); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 21: \
    tmp = fieldname(insn, 5, 7); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 22: \
    tmp = fieldname(insn, 2, 24); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 23: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 18, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 24: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 25: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 26: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 27: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 28: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 1, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 29: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 16); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 16) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 30: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 5, 1) << 5; \
    tmp |= fieldname(insn, 6, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 31: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 5, 1) << 5; \
    tmp |= fieldname(insn, 6, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 32: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 5, 1) << 5; \
    tmp |= fieldname(insn, 6, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 33: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 34: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 35: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 36: \
    tmp = fieldname(insn, 15, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 37: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 11, 5) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 38: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 39: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 40: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeVRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 41: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 42: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 43: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 44: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 45: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 46: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 47: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRC_NOR0RegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeCRBITRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 48: \
    tmp = fieldname(insn, 12, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 49: \
    tmp = fieldname(insn, 12, 8); \
    if (decodeCRBitMOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 50: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 8); \
    if (decodeCRBitMOperand(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 51: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 52: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 53: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 54: \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 55: \
    tmp = 0; \
    tmp |= fieldname(insn, 11, 5) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 56: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 57: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 58: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 59: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 60: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 61: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 62: \
    tmp = fieldname(insn, 21, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 63: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 64: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 65: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 66: \
    tmp = fieldname(insn, 21, 2); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 67: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 68: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 69: \
    tmp = fieldname(insn, 21, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 70: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 71: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 72: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 73: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 74: \
    tmp = fieldname(insn, 21, 2); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 75: \
    tmp = fieldname(insn, 21, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 76: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 77: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 5) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 78: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 79: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 80: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 81: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 6) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 82: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeGPRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 21); \
    if (decodeMemRIOperands(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 83: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 21); \
    if (decodeMemRIOperands(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 84: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 0, 21); \
    if (decodeMemRIOperands(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 85: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeG8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 2, 19); \
    if (decodeMemRIXOperands(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 86: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 87: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 88: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 89: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 90: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 91: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 92: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 93: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 94: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 95: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 8, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 96: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 97: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 98: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 99: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 100: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 101: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 102: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 103: \
    tmp = 0; \
    tmp |= fieldname(insn, 0, 1) << 5; \
    tmp |= fieldname(insn, 21, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 2, 1) << 5; \
    tmp |= fieldname(insn, 16, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 1, 1) << 5; \
    tmp |= fieldname(insn, 11, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = 0; \
    tmp |= fieldname(insn, 3, 1) << 5; \
    tmp |= fieldname(insn, 6, 5) << 0; \
    if (DecodeVSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 104: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 105: \
    tmp = fieldname(insn, 23, 3); \
    if (DecodeCRRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 12, 4); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 106: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 107: \
    tmp = fieldname(insn, 17, 8); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 25, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    tmp = fieldname(insn, 16, 1); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 108: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 109: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 110: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF4RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 111: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 112: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeF8RCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 113: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQBRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 114: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 115: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQBRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQBRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQBRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 7, 4); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 12) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 116: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 9, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 117: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 9, 2); \
    MCOperand_CreateImm0(MI, tmp); \
    return S; \
  case 118: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 9, 12); \
    if (decodeUImmOperand(MI, tmp, Address, Decoder, 12) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 119: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 120: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 121: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 122: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 123: \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodePointerLikeRegClass1(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodePointerLikeRegClass0(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 124: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 125: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 126: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 127: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 128: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 129: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQBRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeQFRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 130: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  case 131: \
    tmp = fieldname(insn, 21, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 16, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 11, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    tmp = fieldname(insn, 6, 5); \
    if (DecodeQSRCRegisterClass(MI, tmp, Address, Decoder) == MCDisassembler_Fail) return MCDisassembler_Fail; \
    return S; \
  } \
}

// DecodeToMCInst(decodeToMCInst_2, fieldFromInstruction_2, uint16_t)
DecodeToMCInst(decodeToMCInst_4, fieldFromInstruction_4, uint32_t)

#define DecodeInstruction(fname, fieldname, decoder, InsnType) \
static DecodeStatus fname(const uint8_t DecodeTable[], MCInst *MI, \
                                      InsnType insn, uint64_t Address, \
                                      int feature) \
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
      return decoder(S, DecodeIdx, insn, MI, Address, MI); \
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

//DecodeInstruction(decodeInstruction_2, fieldFromInstruction_2, decodeToMCInst_2, uint16_t)

DecodeInstruction(decodeInstruction_4, fieldFromInstruction_4, decodeToMCInst_4, uint32_t)
