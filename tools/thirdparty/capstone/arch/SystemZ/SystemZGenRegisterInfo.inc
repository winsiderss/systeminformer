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
  SystemZ_NoRegister,
  SystemZ_CC = 1,
  SystemZ_F0D = 2,
  SystemZ_F1D = 3,
  SystemZ_F2D = 4,
  SystemZ_F3D = 5,
  SystemZ_F4D = 6,
  SystemZ_F5D = 7,
  SystemZ_F6D = 8,
  SystemZ_F7D = 9,
  SystemZ_F8D = 10,
  SystemZ_F9D = 11,
  SystemZ_F10D = 12,
  SystemZ_F11D = 13,
  SystemZ_F12D = 14,
  SystemZ_F13D = 15,
  SystemZ_F14D = 16,
  SystemZ_F15D = 17,
  SystemZ_F0Q = 18,
  SystemZ_F1Q = 19,
  SystemZ_F4Q = 20,
  SystemZ_F5Q = 21,
  SystemZ_F8Q = 22,
  SystemZ_F9Q = 23,
  SystemZ_F12Q = 24,
  SystemZ_F13Q = 25,
  SystemZ_F0S = 26,
  SystemZ_F1S = 27,
  SystemZ_F2S = 28,
  SystemZ_F3S = 29,
  SystemZ_F4S = 30,
  SystemZ_F5S = 31,
  SystemZ_F6S = 32,
  SystemZ_F7S = 33,
  SystemZ_F8S = 34,
  SystemZ_F9S = 35,
  SystemZ_F10S = 36,
  SystemZ_F11S = 37,
  SystemZ_F12S = 38,
  SystemZ_F13S = 39,
  SystemZ_F14S = 40,
  SystemZ_F15S = 41,
  SystemZ_R0D = 42,
  SystemZ_R1D = 43,
  SystemZ_R2D = 44,
  SystemZ_R3D = 45,
  SystemZ_R4D = 46,
  SystemZ_R5D = 47,
  SystemZ_R6D = 48,
  SystemZ_R7D = 49,
  SystemZ_R8D = 50,
  SystemZ_R9D = 51,
  SystemZ_R10D = 52,
  SystemZ_R11D = 53,
  SystemZ_R12D = 54,
  SystemZ_R13D = 55,
  SystemZ_R14D = 56,
  SystemZ_R15D = 57,
  SystemZ_R0H = 58,
  SystemZ_R1H = 59,
  SystemZ_R2H = 60,
  SystemZ_R3H = 61,
  SystemZ_R4H = 62,
  SystemZ_R5H = 63,
  SystemZ_R6H = 64,
  SystemZ_R7H = 65,
  SystemZ_R8H = 66,
  SystemZ_R9H = 67,
  SystemZ_R10H = 68,
  SystemZ_R11H = 69,
  SystemZ_R12H = 70,
  SystemZ_R13H = 71,
  SystemZ_R14H = 72,
  SystemZ_R15H = 73,
  SystemZ_R0L = 74,
  SystemZ_R1L = 75,
  SystemZ_R2L = 76,
  SystemZ_R3L = 77,
  SystemZ_R4L = 78,
  SystemZ_R5L = 79,
  SystemZ_R6L = 80,
  SystemZ_R7L = 81,
  SystemZ_R8L = 82,
  SystemZ_R9L = 83,
  SystemZ_R10L = 84,
  SystemZ_R11L = 85,
  SystemZ_R12L = 86,
  SystemZ_R13L = 87,
  SystemZ_R14L = 88,
  SystemZ_R15L = 89,
  SystemZ_R0Q = 90,
  SystemZ_R2Q = 91,
  SystemZ_R4Q = 92,
  SystemZ_R6Q = 93,
  SystemZ_R8Q = 94,
  SystemZ_R10Q = 95,
  SystemZ_R12Q = 96,
  SystemZ_R14Q = 97,
  SystemZ_NUM_TARGET_REGS 	// 98
};

// Register classes
enum {
  SystemZ_GRX32BitRegClassID = 0,
  SystemZ_FP32BitRegClassID = 1,
  SystemZ_GR32BitRegClassID = 2,
  SystemZ_GRH32BitRegClassID = 3,
  SystemZ_ADDR32BitRegClassID = 4,
  SystemZ_CCRegsRegClassID = 5,
  SystemZ_FP64BitRegClassID = 6,
  SystemZ_GR64BitRegClassID = 7,
  SystemZ_ADDR64BitRegClassID = 8,
  SystemZ_FP128BitRegClassID = 9,
  SystemZ_GR128BitRegClassID = 10,
  SystemZ_ADDR128BitRegClassID = 11
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

static const MCPhysReg SystemZRegDiffLists[] = {
  /* 0 */ 65193, 1, 1, 1, 0,
  /* 5 */ 65469, 1, 0,
  /* 8 */ 65519, 2, 0,
  /* 11 */ 65521, 2, 0,
  /* 14 */ 65523, 2, 0,
  /* 17 */ 65525, 2, 0,
  /* 20 */ 65512, 8, 0,
  /* 23 */ 65512, 10, 0,
  /* 26 */ 65512, 12, 0,
  /* 29 */ 65512, 14, 0,
  /* 32 */ 65512, 16, 0,
  /* 35 */ 65522, 24, 65510, 24, 0,
  /* 40 */ 65524, 24, 65510, 24, 0,
  /* 45 */ 65526, 24, 65510, 24, 0,
  /* 50 */ 65528, 24, 65510, 24, 0,
  /* 55 */ 65504, 40, 0,
  /* 58 */ 65520, 40, 0,
  /* 61 */ 65504, 41, 0,
  /* 64 */ 65520, 41, 0,
  /* 67 */ 65504, 42, 0,
  /* 70 */ 65520, 42, 0,
  /* 73 */ 65504, 43, 0,
  /* 76 */ 65520, 43, 0,
  /* 79 */ 65504, 44, 0,
  /* 82 */ 65520, 44, 0,
  /* 85 */ 65504, 45, 0,
  /* 88 */ 65520, 45, 0,
  /* 91 */ 65504, 46, 0,
  /* 94 */ 65520, 46, 0,
  /* 97 */ 65504, 47, 0,
  /* 100 */ 65520, 47, 0,
  /* 103 */ 65504, 48, 0,
  /* 106 */ 65520, 48, 0,
  /* 109 */ 65405, 0,
  /* 111 */ 65438, 0,
  /* 113 */ 65511, 0,
  /* 115 */ 65489, 32, 65520, 65519, 32, 65520, 0,
  /* 122 */ 65490, 32, 65520, 65519, 32, 65520, 0,
  /* 129 */ 65491, 32, 65520, 65519, 32, 65520, 0,
  /* 136 */ 65492, 32, 65520, 65519, 32, 65520, 0,
  /* 143 */ 65493, 32, 65520, 65519, 32, 65520, 0,
  /* 150 */ 65494, 32, 65520, 65519, 32, 65520, 0,
  /* 157 */ 65495, 32, 65520, 65519, 32, 65520, 0,
  /* 164 */ 65496, 32, 65520, 65519, 32, 65520, 0,
  /* 171 */ 65535, 0,
};

static const uint16_t SystemZSubRegIdxLists[] = {
  /* 0 */ 5, 1, 0,
  /* 3 */ 6, 1, 2, 3, 0,
  /* 8 */ 6, 5, 1, 2, 4, 3, 0,
};

static MCRegisterDesc SystemZRegDesc[] = { // Descriptors
  { 2, 0, 0, 0, 0, 0 },
  { 0, 4, 4, 2, 2737, 0 },
  { 13, 38, 33, 1, 2737, 3 },
  { 31, 38, 33, 1, 2737, 3 },
  { 49, 38, 30, 1, 2737, 3 },
  { 67, 38, 30, 1, 2737, 3 },
  { 85, 38, 30, 1, 2737, 3 },
  { 103, 38, 30, 1, 2737, 3 },
  { 111, 38, 27, 1, 2737, 3 },
  { 119, 38, 27, 1, 2737, 3 },
  { 127, 38, 27, 1, 2737, 3 },
  { 135, 38, 27, 1, 2737, 3 },
  { 3, 38, 24, 1, 2737, 3 },
  { 21, 38, 24, 1, 2737, 3 },
  { 39, 38, 24, 1, 2737, 3 },
  { 57, 38, 24, 1, 2737, 3 },
  { 75, 38, 21, 1, 2737, 3 },
  { 93, 38, 21, 1, 2737, 3 },
  { 288, 35, 4, 3, 129, 2 },
  { 296, 35, 4, 3, 129, 2 },
  { 324, 40, 4, 3, 177, 2 },
  { 332, 40, 4, 3, 177, 2 },
  { 340, 45, 4, 3, 225, 2 },
  { 348, 45, 4, 3, 225, 2 },
  { 300, 50, 4, 3, 273, 2 },
  { 314, 50, 4, 3, 273, 2 },
  { 357, 4, 32, 2, 1809, 0 },
  { 366, 4, 32, 2, 1809, 0 },
  { 375, 4, 29, 2, 1809, 0 },
  { 384, 4, 29, 2, 1809, 0 },
  { 393, 4, 29, 2, 1809, 0 },
  { 402, 4, 29, 2, 1809, 0 },
  { 406, 4, 26, 2, 1809, 0 },
  { 410, 4, 26, 2, 1809, 0 },
  { 414, 4, 26, 2, 1809, 0 },
  { 418, 4, 26, 2, 1809, 0 },
  { 352, 4, 23, 2, 1809, 0 },
  { 361, 4, 23, 2, 1809, 0 },
  { 370, 4, 23, 2, 1809, 0 },
  { 379, 4, 23, 2, 1809, 0 },
  { 388, 4, 20, 2, 1809, 0 },
  { 397, 4, 20, 2, 1809, 0 },
  { 17, 119, 104, 0, 82, 7 },
  { 35, 119, 98, 0, 82, 7 },
  { 53, 119, 98, 0, 82, 7 },
  { 71, 119, 92, 0, 82, 7 },
  { 89, 119, 92, 0, 82, 7 },
  { 107, 119, 86, 0, 82, 7 },
  { 115, 119, 86, 0, 82, 7 },
  { 123, 119, 80, 0, 82, 7 },
  { 131, 119, 80, 0, 82, 7 },
  { 139, 119, 74, 0, 82, 7 },
  { 8, 119, 74, 0, 82, 7 },
  { 26, 119, 68, 0, 82, 7 },
  { 44, 119, 68, 0, 82, 7 },
  { 62, 119, 62, 0, 82, 7 },
  { 80, 119, 62, 0, 82, 7 },
  { 98, 119, 56, 0, 82, 7 },
  { 148, 4, 106, 2, 1778, 0 },
  { 157, 4, 100, 2, 1778, 0 },
  { 166, 4, 100, 2, 1778, 0 },
  { 175, 4, 94, 2, 1778, 0 },
  { 184, 4, 94, 2, 1778, 0 },
  { 193, 4, 88, 2, 1778, 0 },
  { 197, 4, 88, 2, 1778, 0 },
  { 201, 4, 82, 2, 1778, 0 },
  { 205, 4, 82, 2, 1778, 0 },
  { 209, 4, 76, 2, 1778, 0 },
  { 143, 4, 76, 2, 1778, 0 },
  { 152, 4, 70, 2, 1778, 0 },
  { 161, 4, 70, 2, 1778, 0 },
  { 170, 4, 64, 2, 1778, 0 },
  { 179, 4, 64, 2, 1778, 0 },
  { 188, 4, 58, 2, 1778, 0 },
  { 218, 4, 103, 2, 1746, 0 },
  { 227, 4, 97, 2, 1746, 0 },
  { 236, 4, 97, 2, 1746, 0 },
  { 245, 4, 91, 2, 1746, 0 },
  { 254, 4, 91, 2, 1746, 0 },
  { 263, 4, 85, 2, 1746, 0 },
  { 267, 4, 85, 2, 1746, 0 },
  { 271, 4, 79, 2, 1746, 0 },
  { 275, 4, 79, 2, 1746, 0 },
  { 279, 4, 73, 2, 1746, 0 },
  { 213, 4, 73, 2, 1746, 0 },
  { 222, 4, 67, 2, 1746, 0 },
  { 231, 4, 67, 2, 1746, 0 },
  { 240, 4, 61, 2, 1746, 0 },
  { 249, 4, 61, 2, 1746, 0 },
  { 258, 4, 55, 2, 1746, 0 },
  { 292, 115, 4, 8, 4, 5 },
  { 310, 122, 4, 8, 4, 5 },
  { 328, 129, 4, 8, 4, 5 },
  { 336, 136, 4, 8, 4, 5 },
  { 344, 143, 4, 8, 4, 5 },
  { 283, 150, 4, 8, 4, 5 },
  { 305, 157, 4, 8, 4, 5 },
  { 319, 164, 4, 8, 4, 5 },
};

  // GRX32Bit Register Class...
  static const MCPhysReg GRX32Bit[] = {
    SystemZ_R0L, SystemZ_R1L, SystemZ_R2L, SystemZ_R3L, SystemZ_R4L, SystemZ_R5L, SystemZ_R0H, SystemZ_R1H, SystemZ_R2H, SystemZ_R3H, SystemZ_R4H, SystemZ_R5H, SystemZ_R15L, SystemZ_R15H, SystemZ_R14L, SystemZ_R14H, SystemZ_R13L, SystemZ_R13H, SystemZ_R12L, SystemZ_R12H, SystemZ_R11L, SystemZ_R11H, SystemZ_R10L, SystemZ_R10H, SystemZ_R9L, SystemZ_R9H, SystemZ_R8L, SystemZ_R8H, SystemZ_R7L, SystemZ_R7H, SystemZ_R6L, SystemZ_R6H, 
  };

  // GRX32Bit Bit set.
  static const uint8_t GRX32BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // FP32Bit Register Class...
  static const MCPhysReg FP32Bit[] = {
    SystemZ_F0S, SystemZ_F1S, SystemZ_F2S, SystemZ_F3S, SystemZ_F4S, SystemZ_F5S, SystemZ_F6S, SystemZ_F7S, SystemZ_F8S, SystemZ_F9S, SystemZ_F10S, SystemZ_F11S, SystemZ_F12S, SystemZ_F13S, SystemZ_F14S, SystemZ_F15S, 
  };

  // FP32Bit Bit set.
  static const uint8_t FP32BitBits[] = {
    0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // GR32Bit Register Class...
  static const MCPhysReg GR32Bit[] = {
    SystemZ_R0L, SystemZ_R1L, SystemZ_R2L, SystemZ_R3L, SystemZ_R4L, SystemZ_R5L, SystemZ_R15L, SystemZ_R14L, SystemZ_R13L, SystemZ_R12L, SystemZ_R11L, SystemZ_R10L, SystemZ_R9L, SystemZ_R8L, SystemZ_R7L, SystemZ_R6L, 
  };

  // GR32Bit Bit set.
  static const uint8_t GR32BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // GRH32Bit Register Class...
  static const MCPhysReg GRH32Bit[] = {
    SystemZ_R0H, SystemZ_R1H, SystemZ_R2H, SystemZ_R3H, SystemZ_R4H, SystemZ_R5H, SystemZ_R15H, SystemZ_R14H, SystemZ_R13H, SystemZ_R12H, SystemZ_R11H, SystemZ_R10H, SystemZ_R9H, SystemZ_R8H, SystemZ_R7H, SystemZ_R6H, 
  };

  // GRH32Bit Bit set.
  static const uint8_t GRH32BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // ADDR32Bit Register Class...
  static const MCPhysReg ADDR32Bit[] = {
    SystemZ_R1L, SystemZ_R2L, SystemZ_R3L, SystemZ_R4L, SystemZ_R5L, SystemZ_R15L, SystemZ_R14L, SystemZ_R13L, SystemZ_R12L, SystemZ_R11L, SystemZ_R10L, SystemZ_R9L, SystemZ_R8L, SystemZ_R7L, SystemZ_R6L, 
  };

  // ADDR32Bit Bit set.
  static const uint8_t ADDR32BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x03, 
  };

  // CCRegs Register Class...
  static const MCPhysReg CCRegs[] = {
    SystemZ_CC, 
  };

  // CCRegs Bit set.
  static const uint8_t CCRegsBits[] = {
    0x02, 
  };

  // FP64Bit Register Class...
  static const MCPhysReg FP64Bit[] = {
    SystemZ_F0D, SystemZ_F1D, SystemZ_F2D, SystemZ_F3D, SystemZ_F4D, SystemZ_F5D, SystemZ_F6D, SystemZ_F7D, SystemZ_F8D, SystemZ_F9D, SystemZ_F10D, SystemZ_F11D, SystemZ_F12D, SystemZ_F13D, SystemZ_F14D, SystemZ_F15D, 
  };

  // FP64Bit Bit set.
  static const uint8_t FP64BitBits[] = {
    0xfc, 0xff, 0x03, 
  };

  // GR64Bit Register Class...
  static const MCPhysReg GR64Bit[] = {
    SystemZ_R0D, SystemZ_R1D, SystemZ_R2D, SystemZ_R3D, SystemZ_R4D, SystemZ_R5D, SystemZ_R15D, SystemZ_R14D, SystemZ_R13D, SystemZ_R12D, SystemZ_R11D, SystemZ_R10D, SystemZ_R9D, SystemZ_R8D, SystemZ_R7D, SystemZ_R6D, 
  };

  // GR64Bit Bit set.
  static const uint8_t GR64BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // ADDR64Bit Register Class...
  static const MCPhysReg ADDR64Bit[] = {
    SystemZ_R1D, SystemZ_R2D, SystemZ_R3D, SystemZ_R4D, SystemZ_R5D, SystemZ_R15D, SystemZ_R14D, SystemZ_R13D, SystemZ_R12D, SystemZ_R11D, SystemZ_R10D, SystemZ_R9D, SystemZ_R8D, SystemZ_R7D, SystemZ_R6D, 
  };

  // ADDR64Bit Bit set.
  static const uint8_t ADDR64BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x03, 
  };

  // FP128Bit Register Class...
  static const MCPhysReg FP128Bit[] = {
    SystemZ_F0Q, SystemZ_F1Q, SystemZ_F4Q, SystemZ_F5Q, SystemZ_F8Q, SystemZ_F9Q, SystemZ_F12Q, SystemZ_F13Q, 
  };

  // FP128Bit Bit set.
  static const uint8_t FP128BitBits[] = {
    0x00, 0x00, 0xfc, 0x03, 
  };

  // GR128Bit Register Class...
  static const MCPhysReg GR128Bit[] = {
    SystemZ_R0Q, SystemZ_R2Q, SystemZ_R4Q, SystemZ_R12Q, SystemZ_R10Q, SystemZ_R8Q, SystemZ_R6Q, SystemZ_R14Q, 
  };

  // GR128Bit Bit set.
  static const uint8_t GR128BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // ADDR128Bit Register Class...
  static const MCPhysReg ADDR128Bit[] = {
    SystemZ_R2Q, SystemZ_R4Q, SystemZ_R12Q, SystemZ_R10Q, SystemZ_R8Q, SystemZ_R6Q, SystemZ_R14Q, 
  };

  // ADDR128Bit Bit set.
  static const uint8_t ADDR128BitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

static MCRegisterClass SystemZMCRegisterClasses[] = {
  { GRX32Bit, GRX32BitBits, 42, 32, sizeof(GRX32BitBits), SystemZ_GRX32BitRegClassID, 4, 4, 1, 1 },
  { FP32Bit, FP32BitBits, 16, 16, sizeof(FP32BitBits), SystemZ_FP32BitRegClassID, 4, 4, 1, 1 },
  { GR32Bit, GR32BitBits, 34, 16, sizeof(GR32BitBits), SystemZ_GR32BitRegClassID, 4, 4, 1, 1 },
  { GRH32Bit, GRH32BitBits, 7, 16, sizeof(GRH32BitBits), SystemZ_GRH32BitRegClassID, 4, 4, 1, 1 },
  { ADDR32Bit, ADDR32BitBits, 24, 15, sizeof(ADDR32BitBits), SystemZ_ADDR32BitRegClassID, 4, 4, 1, 1 },
  { CCRegs, CCRegsBits, 0, 1, sizeof(CCRegsBits), SystemZ_CCRegsRegClassID, 4, 4, 1, 1 },
  { FP64Bit, FP64BitBits, 51, 16, sizeof(FP64BitBits), SystemZ_FP64BitRegClassID, 8, 8, 1, 1 },
  { GR64Bit, GR64BitBits, 69, 16, sizeof(GR64BitBits), SystemZ_GR64BitRegClassID, 8, 8, 1, 1 },
  { ADDR64Bit, ADDR64BitBits, 59, 15, sizeof(ADDR64BitBits), SystemZ_ADDR64BitRegClassID, 8, 8, 1, 1 },
  { FP128Bit, FP128BitBits, 77, 8, sizeof(FP128BitBits), SystemZ_FP128BitRegClassID, 16, 16, 1, 1 },
  { GR128Bit, GR128BitBits, 97, 8, sizeof(GR128BitBits), SystemZ_GR128BitRegClassID, 16, 16, 1, 1 },
  { ADDR128Bit, ADDR128BitBits, 86, 7, sizeof(ADDR128BitBits), SystemZ_ADDR128BitRegClassID, 16, 16, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC
