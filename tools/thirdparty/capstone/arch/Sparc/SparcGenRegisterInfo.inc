/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|*Target Register Enum Values                                                 *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */


#ifdef GET_REGINFO_ENUM
#undef GET_REGINFO_ENUM

enum {
  SP_NoRegister,
  SP_ICC = 1,
  SP_Y = 2,
  SP_D0 = 3,
  SP_D1 = 4,
  SP_D2 = 5,
  SP_D3 = 6,
  SP_D4 = 7,
  SP_D5 = 8,
  SP_D6 = 9,
  SP_D7 = 10,
  SP_D8 = 11,
  SP_D9 = 12,
  SP_D10 = 13,
  SP_D11 = 14,
  SP_D12 = 15,
  SP_D13 = 16,
  SP_D14 = 17,
  SP_D15 = 18,
  SP_D16 = 19,
  SP_D17 = 20,
  SP_D18 = 21,
  SP_D19 = 22,
  SP_D20 = 23,
  SP_D21 = 24,
  SP_D22 = 25,
  SP_D23 = 26,
  SP_D24 = 27,
  SP_D25 = 28,
  SP_D26 = 29,
  SP_D27 = 30,
  SP_D28 = 31,
  SP_D29 = 32,
  SP_D30 = 33,
  SP_D31 = 34,
  SP_F0 = 35,
  SP_F1 = 36,
  SP_F2 = 37,
  SP_F3 = 38,
  SP_F4 = 39,
  SP_F5 = 40,
  SP_F6 = 41,
  SP_F7 = 42,
  SP_F8 = 43,
  SP_F9 = 44,
  SP_F10 = 45,
  SP_F11 = 46,
  SP_F12 = 47,
  SP_F13 = 48,
  SP_F14 = 49,
  SP_F15 = 50,
  SP_F16 = 51,
  SP_F17 = 52,
  SP_F18 = 53,
  SP_F19 = 54,
  SP_F20 = 55,
  SP_F21 = 56,
  SP_F22 = 57,
  SP_F23 = 58,
  SP_F24 = 59,
  SP_F25 = 60,
  SP_F26 = 61,
  SP_F27 = 62,
  SP_F28 = 63,
  SP_F29 = 64,
  SP_F30 = 65,
  SP_F31 = 66,
  SP_FCC0 = 67,
  SP_FCC1 = 68,
  SP_FCC2 = 69,
  SP_FCC3 = 70,
  SP_G0 = 71,
  SP_G1 = 72,
  SP_G2 = 73,
  SP_G3 = 74,
  SP_G4 = 75,
  SP_G5 = 76,
  SP_G6 = 77,
  SP_G7 = 78,
  SP_I0 = 79,
  SP_I1 = 80,
  SP_I2 = 81,
  SP_I3 = 82,
  SP_I4 = 83,
  SP_I5 = 84,
  SP_I6 = 85,
  SP_I7 = 86,
  SP_L0 = 87,
  SP_L1 = 88,
  SP_L2 = 89,
  SP_L3 = 90,
  SP_L4 = 91,
  SP_L5 = 92,
  SP_L6 = 93,
  SP_L7 = 94,
  SP_O0 = 95,
  SP_O1 = 96,
  SP_O2 = 97,
  SP_O3 = 98,
  SP_O4 = 99,
  SP_O5 = 100,
  SP_O6 = 101,
  SP_O7 = 102,
  SP_Q0 = 103,
  SP_Q1 = 104,
  SP_Q2 = 105,
  SP_Q3 = 106,
  SP_Q4 = 107,
  SP_Q5 = 108,
  SP_Q6 = 109,
  SP_Q7 = 110,
  SP_Q8 = 111,
  SP_Q9 = 112,
  SP_Q10 = 113,
  SP_Q11 = 114,
  SP_Q12 = 115,
  SP_Q13 = 116,
  SP_Q14 = 117,
  SP_Q15 = 118,
  SP_NUM_TARGET_REGS 	// 119
};

// Register classes
enum {
  SP_FCCRegsRegClassID = 0,
  SP_FPRegsRegClassID = 1,
  SP_IntRegsRegClassID = 2,
  SP_DFPRegsRegClassID = 3,
  SP_I64RegsRegClassID = 4,
  SP_DFPRegs_with_sub_evenRegClassID = 5,
  SP_QFPRegsRegClassID = 6,
  SP_QFPRegs_with_sub_evenRegClassID = 7
};

#endif // GET_REGINFO_ENUM

/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|*MC Register Information                                                     *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/


#ifdef GET_REGINFO_MC_DESC
#undef GET_REGINFO_MC_DESC

static const MCPhysReg SparcRegDiffLists[] = {
  /* 0 */ 65126, 1, 1, 1, 0,
  /* 5 */ 32, 1, 0,
  /* 8 */ 65436, 32, 1, 65504, 33, 1, 0,
  /* 15 */ 34, 1, 0,
  /* 18 */ 65437, 34, 1, 65502, 35, 1, 0,
  /* 25 */ 36, 1, 0,
  /* 28 */ 65438, 36, 1, 65500, 37, 1, 0,
  /* 35 */ 38, 1, 0,
  /* 38 */ 65439, 38, 1, 65498, 39, 1, 0,
  /* 45 */ 40, 1, 0,
  /* 48 */ 65440, 40, 1, 65496, 41, 1, 0,
  /* 55 */ 42, 1, 0,
  /* 58 */ 65441, 42, 1, 65494, 43, 1, 0,
  /* 65 */ 44, 1, 0,
  /* 68 */ 65442, 44, 1, 65492, 45, 1, 0,
  /* 75 */ 46, 1, 0,
  /* 78 */ 65443, 46, 1, 65490, 47, 1, 0,
  /* 85 */ 65348, 1, 0,
  /* 88 */ 65444, 1, 0,
  /* 91 */ 65445, 1, 0,
  /* 94 */ 65446, 1, 0,
  /* 97 */ 65447, 1, 0,
  /* 100 */ 65448, 1, 0,
  /* 103 */ 65449, 1, 0,
  /* 106 */ 65450, 1, 0,
  /* 109 */ 65451, 1, 0,
  /* 112 */ 65532, 1, 0,
  /* 115 */ 15, 0,
  /* 117 */ 84, 0,
  /* 119 */ 85, 0,
  /* 121 */ 86, 0,
  /* 123 */ 87, 0,
  /* 125 */ 88, 0,
  /* 127 */ 89, 0,
  /* 129 */ 90, 0,
  /* 131 */ 91, 0,
  /* 133 */ 65488, 92, 0,
  /* 136 */ 65489, 92, 0,
  /* 139 */ 65489, 93, 0,
  /* 142 */ 65490, 93, 0,
  /* 145 */ 65491, 93, 0,
  /* 148 */ 65491, 94, 0,
  /* 151 */ 65492, 94, 0,
  /* 154 */ 65493, 94, 0,
  /* 157 */ 65493, 95, 0,
  /* 160 */ 65494, 95, 0,
  /* 163 */ 65495, 95, 0,
  /* 166 */ 65495, 96, 0,
  /* 169 */ 65496, 96, 0,
  /* 172 */ 65497, 96, 0,
  /* 175 */ 65497, 97, 0,
  /* 178 */ 65498, 97, 0,
  /* 181 */ 65499, 97, 0,
  /* 184 */ 65499, 98, 0,
  /* 187 */ 65500, 98, 0,
  /* 190 */ 65501, 98, 0,
  /* 193 */ 65501, 99, 0,
  /* 196 */ 65502, 99, 0,
  /* 199 */ 65503, 99, 0,
  /* 202 */ 65503, 100, 0,
  /* 205 */ 65504, 100, 0,
  /* 208 */ 65503, 0,
  /* 210 */ 65519, 0,
  /* 212 */ 65535, 0,
};

static const uint16_t SparcSubRegIdxLists[] = {
  /* 0 */ 1, 3, 0,
  /* 3 */ 2, 4, 0,
  /* 6 */ 2, 1, 3, 4, 5, 6, 0,
};

static MCRegisterDesc SparcRegDesc[] = { // Descriptors
  { 3, 0, 0, 0, 0, 0 },
  { 406, 4, 4, 2, 3393, 0 },
  { 410, 4, 4, 2, 3393, 0 },
  { 33, 5, 203, 0, 1794, 2 },
  { 87, 12, 194, 0, 1794, 2 },
  { 133, 15, 194, 0, 1794, 2 },
  { 179, 22, 185, 0, 1794, 2 },
  { 220, 25, 185, 0, 1794, 2 },
  { 261, 32, 176, 0, 1794, 2 },
  { 298, 35, 176, 0, 1794, 2 },
  { 335, 42, 167, 0, 1794, 2 },
  { 372, 45, 167, 0, 1794, 2 },
  { 397, 52, 158, 0, 1794, 2 },
  { 0, 55, 158, 0, 1794, 2 },
  { 54, 62, 149, 0, 1794, 2 },
  { 108, 65, 149, 0, 1794, 2 },
  { 154, 72, 140, 0, 1794, 2 },
  { 200, 75, 140, 0, 1794, 2 },
  { 241, 82, 134, 0, 1794, 2 },
  { 282, 4, 134, 2, 1841, 0 },
  { 319, 4, 131, 2, 1841, 0 },
  { 356, 4, 131, 2, 1841, 0 },
  { 381, 4, 129, 2, 1841, 0 },
  { 12, 4, 129, 2, 1841, 0 },
  { 66, 4, 127, 2, 1841, 0 },
  { 120, 4, 127, 2, 1841, 0 },
  { 166, 4, 125, 2, 1841, 0 },
  { 212, 4, 125, 2, 1841, 0 },
  { 253, 4, 123, 2, 1841, 0 },
  { 290, 4, 123, 2, 1841, 0 },
  { 327, 4, 121, 2, 1841, 0 },
  { 364, 4, 121, 2, 1841, 0 },
  { 389, 4, 119, 2, 1841, 0 },
  { 20, 4, 119, 2, 1841, 0 },
  { 74, 4, 117, 2, 1841, 0 },
  { 36, 4, 205, 2, 3329, 0 },
  { 90, 4, 202, 2, 3329, 0 },
  { 136, 4, 199, 2, 3329, 0 },
  { 182, 4, 196, 2, 3329, 0 },
  { 223, 4, 196, 2, 3329, 0 },
  { 264, 4, 193, 2, 3329, 0 },
  { 301, 4, 190, 2, 3329, 0 },
  { 338, 4, 187, 2, 3329, 0 },
  { 375, 4, 187, 2, 3329, 0 },
  { 400, 4, 184, 2, 3329, 0 },
  { 4, 4, 181, 2, 3329, 0 },
  { 58, 4, 178, 2, 3329, 0 },
  { 112, 4, 178, 2, 3329, 0 },
  { 158, 4, 175, 2, 3329, 0 },
  { 204, 4, 172, 2, 3329, 0 },
  { 245, 4, 169, 2, 3329, 0 },
  { 286, 4, 169, 2, 3329, 0 },
  { 323, 4, 166, 2, 3329, 0 },
  { 360, 4, 163, 2, 3329, 0 },
  { 385, 4, 160, 2, 3329, 0 },
  { 16, 4, 160, 2, 3329, 0 },
  { 70, 4, 157, 2, 3329, 0 },
  { 124, 4, 154, 2, 3329, 0 },
  { 170, 4, 151, 2, 3329, 0 },
  { 216, 4, 151, 2, 3329, 0 },
  { 257, 4, 148, 2, 3329, 0 },
  { 294, 4, 145, 2, 3329, 0 },
  { 331, 4, 142, 2, 3329, 0 },
  { 368, 4, 142, 2, 3329, 0 },
  { 393, 4, 139, 2, 3329, 0 },
  { 24, 4, 136, 2, 3329, 0 },
  { 78, 4, 133, 2, 3329, 0 },
  { 28, 4, 4, 2, 3361, 0 },
  { 82, 4, 4, 2, 3361, 0 },
  { 128, 4, 4, 2, 3361, 0 },
  { 174, 4, 4, 2, 3361, 0 },
  { 39, 4, 4, 2, 3361, 0 },
  { 93, 4, 4, 2, 3361, 0 },
  { 139, 4, 4, 2, 3361, 0 },
  { 185, 4, 4, 2, 3361, 0 },
  { 226, 4, 4, 2, 3361, 0 },
  { 267, 4, 4, 2, 3361, 0 },
  { 304, 4, 4, 2, 3361, 0 },
  { 341, 4, 4, 2, 3361, 0 },
  { 42, 4, 4, 2, 3361, 0 },
  { 96, 4, 4, 2, 3361, 0 },
  { 142, 4, 4, 2, 3361, 0 },
  { 188, 4, 4, 2, 3361, 0 },
  { 229, 4, 4, 2, 3361, 0 },
  { 270, 4, 4, 2, 3361, 0 },
  { 307, 4, 4, 2, 3361, 0 },
  { 344, 4, 4, 2, 3361, 0 },
  { 45, 4, 4, 2, 3361, 0 },
  { 99, 4, 4, 2, 3361, 0 },
  { 145, 4, 4, 2, 3361, 0 },
  { 191, 4, 4, 2, 3361, 0 },
  { 232, 4, 4, 2, 3361, 0 },
  { 273, 4, 4, 2, 3361, 0 },
  { 310, 4, 4, 2, 3361, 0 },
  { 347, 4, 4, 2, 3361, 0 },
  { 48, 4, 4, 2, 3361, 0 },
  { 102, 4, 4, 2, 3361, 0 },
  { 148, 4, 4, 2, 3361, 0 },
  { 194, 4, 4, 2, 3361, 0 },
  { 235, 4, 4, 2, 3361, 0 },
  { 276, 4, 4, 2, 3361, 0 },
  { 313, 4, 4, 2, 3361, 0 },
  { 350, 4, 4, 2, 3361, 0 },
  { 51, 8, 4, 6, 4, 5 },
  { 105, 18, 4, 6, 4, 5 },
  { 151, 28, 4, 6, 4, 5 },
  { 197, 38, 4, 6, 4, 5 },
  { 238, 48, 4, 6, 4, 5 },
  { 279, 58, 4, 6, 4, 5 },
  { 316, 68, 4, 6, 4, 5 },
  { 353, 78, 4, 6, 4, 5 },
  { 378, 88, 4, 3, 1362, 10 },
  { 403, 91, 4, 3, 1362, 10 },
  { 8, 94, 4, 3, 1362, 10 },
  { 62, 97, 4, 3, 1362, 10 },
  { 116, 100, 4, 3, 1362, 10 },
  { 162, 103, 4, 3, 1362, 10 },
  { 208, 106, 4, 3, 1362, 10 },
  { 249, 109, 4, 3, 1362, 10 },
};

  // FCCRegs Register Class...
  static const MCPhysReg FCCRegs[] = {
    SP_FCC0, SP_FCC1, SP_FCC2, SP_FCC3, 
  };

  // FCCRegs Bit set.
  static const uint8_t FCCRegsBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 
  };

  // FPRegs Register Class...
  static const MCPhysReg FPRegs[] = {
    SP_F0, SP_F1, SP_F2, SP_F3, SP_F4, SP_F5, SP_F6, SP_F7, SP_F8, SP_F9, SP_F10, SP_F11, SP_F12, SP_F13, SP_F14, SP_F15, SP_F16, SP_F17, SP_F18, SP_F19, SP_F20, SP_F21, SP_F22, SP_F23, SP_F24, SP_F25, SP_F26, SP_F27, SP_F28, SP_F29, SP_F30, SP_F31, 
  };

  // FPRegs Bit set.
  static const uint8_t FPRegsBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x07, 
  };

  // IntRegs Register Class...
  static const MCPhysReg IntRegs[] = {
    SP_I0, SP_I1, SP_I2, SP_I3, SP_I4, SP_I5, SP_I6, SP_I7, SP_G0, SP_G1, SP_G2, SP_G3, SP_G4, SP_G5, SP_G6, SP_G7, SP_L0, SP_L1, SP_L2, SP_L3, SP_L4, SP_L5, SP_L6, SP_L7, SP_O0, SP_O1, SP_O2, SP_O3, SP_O4, SP_O5, SP_O6, SP_O7, 
  };

  // IntRegs Bit set.
  static const uint8_t IntRegsBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x7f, 
  };

  // DFPRegs Register Class...
  static const MCPhysReg DFPRegs[] = {
    SP_D0, SP_D1, SP_D2, SP_D3, SP_D4, SP_D5, SP_D6, SP_D7, SP_D8, SP_D9, SP_D10, SP_D11, SP_D12, SP_D13, SP_D14, SP_D15, SP_D16, SP_D17, SP_D18, SP_D19, SP_D20, SP_D21, SP_D22, SP_D23, SP_D24, SP_D25, SP_D26, SP_D27, SP_D28, SP_D29, SP_D30, SP_D31, 
  };

  // DFPRegs Bit set.
  static const uint8_t DFPRegsBits[] = {
    0xf8, 0xff, 0xff, 0xff, 0x07, 
  };

  // I64Regs Register Class...
  static const MCPhysReg I64Regs[] = {
    SP_I0, SP_I1, SP_I2, SP_I3, SP_I4, SP_I5, SP_I6, SP_I7, SP_G0, SP_G1, SP_G2, SP_G3, SP_G4, SP_G5, SP_G6, SP_G7, SP_L0, SP_L1, SP_L2, SP_L3, SP_L4, SP_L5, SP_L6, SP_L7, SP_O0, SP_O1, SP_O2, SP_O3, SP_O4, SP_O5, SP_O6, SP_O7, 
  };

  // I64Regs Bit set.
  static const uint8_t I64RegsBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x7f, 
  };

  // DFPRegs_with_sub_even Register Class...
  static const MCPhysReg DFPRegs_with_sub_even[] = {
    SP_D0, SP_D1, SP_D2, SP_D3, SP_D4, SP_D5, SP_D6, SP_D7, SP_D8, SP_D9, SP_D10, SP_D11, SP_D12, SP_D13, SP_D14, SP_D15, 
  };

  // DFPRegs_with_sub_even Bit set.
  static const uint8_t DFPRegs_with_sub_evenBits[] = {
    0xf8, 0xff, 0x07, 
  };

  // QFPRegs Register Class...
  static const MCPhysReg QFPRegs[] = {
    SP_Q0, SP_Q1, SP_Q2, SP_Q3, SP_Q4, SP_Q5, SP_Q6, SP_Q7, SP_Q8, SP_Q9, SP_Q10, SP_Q11, SP_Q12, SP_Q13, SP_Q14, SP_Q15, 
  };

  // QFPRegs Bit set.
  static const uint8_t QFPRegsBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x7f, 
  };

  // QFPRegs_with_sub_even Register Class...
  static const MCPhysReg QFPRegs_with_sub_even[] = {
    SP_Q0, SP_Q1, SP_Q2, SP_Q3, SP_Q4, SP_Q5, SP_Q6, SP_Q7, 
  };

  // QFPRegs_with_sub_even Bit set.
  static const uint8_t QFPRegs_with_sub_evenBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x7f, 
  };

static MCRegisterClass SparcMCRegisterClasses[] = {
  { FCCRegs, FCCRegsBits, 52, 4, sizeof(FCCRegsBits), SP_FCCRegsRegClassID, 0, 0, 1, 1 },
  { FPRegs, FPRegsBits, 61, 32, sizeof(FPRegsBits), SP_FPRegsRegClassID, 4, 4, 1, 1 },
  { IntRegs, IntRegsBits, 76, 32, sizeof(IntRegsBits), SP_IntRegsRegClassID, 4, 4, 1, 1 },
  { DFPRegs, DFPRegsBits, 60, 32, sizeof(DFPRegsBits), SP_DFPRegsRegClassID, 8, 8, 1, 1 },
  { I64Regs, I64RegsBits, 44, 32, sizeof(I64RegsBits), SP_I64RegsRegClassID, 8, 8, 1, 1 },
  { DFPRegs_with_sub_even, DFPRegs_with_sub_evenBits, 0, 16, sizeof(DFPRegs_with_sub_evenBits), SP_DFPRegs_with_sub_evenRegClassID, 8, 8, 1, 1 },
  { QFPRegs, QFPRegsBits, 68, 16, sizeof(QFPRegsBits), SP_QFPRegsRegClassID, 16, 16, 1, 1 },
  { QFPRegs_with_sub_even, QFPRegs_with_sub_evenBits, 22, 8, sizeof(QFPRegs_with_sub_evenBits), SP_QFPRegs_with_sub_evenRegClassID, 16, 16, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC
