/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|*Target Register Enum Values                                                 *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* Capstone Disassembly Engine, http://www.capstone-engine.org */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */


#ifdef GET_REGINFO_ENUM
#undef GET_REGINFO_ENUM

enum {
  X86_NoRegister,
  X86_AH = 1,
  X86_AL = 2,
  X86_AX = 3,
  X86_BH = 4,
  X86_BL = 5,
  X86_BP = 6,
  X86_BPL = 7,
  X86_BX = 8,
  X86_CH = 9,
  X86_CL = 10,
  X86_CS = 11,
  X86_CX = 12,
  X86_DH = 13,
  X86_DI = 14,
  X86_DIL = 15,
  X86_DL = 16,
  X86_DS = 17,
  X86_DX = 18,
  X86_EAX = 19,
  X86_EBP = 20,
  X86_EBX = 21,
  X86_ECX = 22,
  X86_EDI = 23,
  X86_EDX = 24,
  X86_EFLAGS = 25,
  X86_EIP = 26,
  X86_EIZ = 27,
  X86_ES = 28,
  X86_ESI = 29,
  X86_ESP = 30,
  X86_FPSW = 31,
  X86_FS = 32,
  X86_GS = 33,
  X86_IP = 34,
  X86_RAX = 35,
  X86_RBP = 36,
  X86_RBX = 37,
  X86_RCX = 38,
  X86_RDI = 39,
  X86_RDX = 40,
  X86_RIP = 41,
  X86_RIZ = 42,
  X86_RSI = 43,
  X86_RSP = 44,
  X86_SI = 45,
  X86_SIL = 46,
  X86_SP = 47,
  X86_SPL = 48,
  X86_SS = 49,
  X86_CR0 = 50,
  X86_CR1 = 51,
  X86_CR2 = 52,
  X86_CR3 = 53,
  X86_CR4 = 54,
  X86_CR5 = 55,
  X86_CR6 = 56,
  X86_CR7 = 57,
  X86_CR8 = 58,
  X86_CR9 = 59,
  X86_CR10 = 60,
  X86_CR11 = 61,
  X86_CR12 = 62,
  X86_CR13 = 63,
  X86_CR14 = 64,
  X86_CR15 = 65,
  X86_DR0 = 66,
  X86_DR1 = 67,
  X86_DR2 = 68,
  X86_DR3 = 69,
  X86_DR4 = 70,
  X86_DR5 = 71,
  X86_DR6 = 72,
  X86_DR7 = 73,
  X86_DR8 = 74,
  X86_DR9 = 75,
  X86_DR10 = 76,
  X86_DR11 = 77,
  X86_DR12 = 78,
  X86_DR13 = 79,
  X86_DR14 = 80,
  X86_DR15 = 81,
  X86_FP0 = 82,
  X86_FP1 = 83,
  X86_FP2 = 84,
  X86_FP3 = 85,
  X86_FP4 = 86,
  X86_FP5 = 87,
  X86_FP6 = 88,
  X86_FP7 = 89,
  X86_K0 = 90,
  X86_K1 = 91,
  X86_K2 = 92,
  X86_K3 = 93,
  X86_K4 = 94,
  X86_K5 = 95,
  X86_K6 = 96,
  X86_K7 = 97,
  X86_MM0 = 98,
  X86_MM1 = 99,
  X86_MM2 = 100,
  X86_MM3 = 101,
  X86_MM4 = 102,
  X86_MM5 = 103,
  X86_MM6 = 104,
  X86_MM7 = 105,
  X86_R8 = 106,
  X86_R9 = 107,
  X86_R10 = 108,
  X86_R11 = 109,
  X86_R12 = 110,
  X86_R13 = 111,
  X86_R14 = 112,
  X86_R15 = 113,
  X86_ST0 = 114,
  X86_ST1 = 115,
  X86_ST2 = 116,
  X86_ST3 = 117,
  X86_ST4 = 118,
  X86_ST5 = 119,
  X86_ST6 = 120,
  X86_ST7 = 121,
  X86_XMM0 = 122,
  X86_XMM1 = 123,
  X86_XMM2 = 124,
  X86_XMM3 = 125,
  X86_XMM4 = 126,
  X86_XMM5 = 127,
  X86_XMM6 = 128,
  X86_XMM7 = 129,
  X86_XMM8 = 130,
  X86_XMM9 = 131,
  X86_XMM10 = 132,
  X86_XMM11 = 133,
  X86_XMM12 = 134,
  X86_XMM13 = 135,
  X86_XMM14 = 136,
  X86_XMM15 = 137,
  X86_XMM16 = 138,
  X86_XMM17 = 139,
  X86_XMM18 = 140,
  X86_XMM19 = 141,
  X86_XMM20 = 142,
  X86_XMM21 = 143,
  X86_XMM22 = 144,
  X86_XMM23 = 145,
  X86_XMM24 = 146,
  X86_XMM25 = 147,
  X86_XMM26 = 148,
  X86_XMM27 = 149,
  X86_XMM28 = 150,
  X86_XMM29 = 151,
  X86_XMM30 = 152,
  X86_XMM31 = 153,
  X86_YMM0 = 154,
  X86_YMM1 = 155,
  X86_YMM2 = 156,
  X86_YMM3 = 157,
  X86_YMM4 = 158,
  X86_YMM5 = 159,
  X86_YMM6 = 160,
  X86_YMM7 = 161,
  X86_YMM8 = 162,
  X86_YMM9 = 163,
  X86_YMM10 = 164,
  X86_YMM11 = 165,
  X86_YMM12 = 166,
  X86_YMM13 = 167,
  X86_YMM14 = 168,
  X86_YMM15 = 169,
  X86_YMM16 = 170,
  X86_YMM17 = 171,
  X86_YMM18 = 172,
  X86_YMM19 = 173,
  X86_YMM20 = 174,
  X86_YMM21 = 175,
  X86_YMM22 = 176,
  X86_YMM23 = 177,
  X86_YMM24 = 178,
  X86_YMM25 = 179,
  X86_YMM26 = 180,
  X86_YMM27 = 181,
  X86_YMM28 = 182,
  X86_YMM29 = 183,
  X86_YMM30 = 184,
  X86_YMM31 = 185,
  X86_ZMM0 = 186,
  X86_ZMM1 = 187,
  X86_ZMM2 = 188,
  X86_ZMM3 = 189,
  X86_ZMM4 = 190,
  X86_ZMM5 = 191,
  X86_ZMM6 = 192,
  X86_ZMM7 = 193,
  X86_ZMM8 = 194,
  X86_ZMM9 = 195,
  X86_ZMM10 = 196,
  X86_ZMM11 = 197,
  X86_ZMM12 = 198,
  X86_ZMM13 = 199,
  X86_ZMM14 = 200,
  X86_ZMM15 = 201,
  X86_ZMM16 = 202,
  X86_ZMM17 = 203,
  X86_ZMM18 = 204,
  X86_ZMM19 = 205,
  X86_ZMM20 = 206,
  X86_ZMM21 = 207,
  X86_ZMM22 = 208,
  X86_ZMM23 = 209,
  X86_ZMM24 = 210,
  X86_ZMM25 = 211,
  X86_ZMM26 = 212,
  X86_ZMM27 = 213,
  X86_ZMM28 = 214,
  X86_ZMM29 = 215,
  X86_ZMM30 = 216,
  X86_ZMM31 = 217,
  X86_R8B = 218,
  X86_R9B = 219,
  X86_R10B = 220,
  X86_R11B = 221,
  X86_R12B = 222,
  X86_R13B = 223,
  X86_R14B = 224,
  X86_R15B = 225,
  X86_R8D = 226,
  X86_R9D = 227,
  X86_R10D = 228,
  X86_R11D = 229,
  X86_R12D = 230,
  X86_R13D = 231,
  X86_R14D = 232,
  X86_R15D = 233,
  X86_R8W = 234,
  X86_R9W = 235,
  X86_R10W = 236,
  X86_R11W = 237,
  X86_R12W = 238,
  X86_R13W = 239,
  X86_R14W = 240,
  X86_R15W = 241,
  X86_NUM_TARGET_REGS 	// 242
};

// Register classes
enum {
  X86_GR8RegClassID = 0,
  X86_GR8_NOREXRegClassID = 1,
  X86_VK1RegClassID = 2,
  X86_VK2RegClassID = 3,
  X86_VK4RegClassID = 4,
  X86_VK8RegClassID = 5,
  X86_VK1WMRegClassID = 6,
  X86_VK2WMRegClassID = 7,
  X86_VK4WMRegClassID = 8,
  X86_VK8WMRegClassID = 9,
  X86_GR8_ABCD_HRegClassID = 10,
  X86_GR8_ABCD_LRegClassID = 11,
  X86_GR16RegClassID = 12,
  X86_GR16_NOREXRegClassID = 13,
  X86_VK16RegClassID = 14,
  X86_VK16WMRegClassID = 15,
  X86_SEGMENT_REGRegClassID = 16,
  X86_GR16_ABCDRegClassID = 17,
  X86_FPCCRRegClassID = 18,
  X86_FR32XRegClassID = 19,
  X86_FR32RegClassID = 20,
  X86_GR32RegClassID = 21,
  X86_GR32_NOAXRegClassID = 22,
  X86_GR32_NOSPRegClassID = 23,
  X86_GR32_NOAX_and_GR32_NOSPRegClassID = 24,
  X86_DEBUG_REGRegClassID = 25,
  X86_GR32_NOREXRegClassID = 26,
  X86_VK32RegClassID = 27,
  X86_GR32_NOAX_and_GR32_NOREXRegClassID = 28,
  X86_GR32_NOREX_NOSPRegClassID = 29,
  X86_RFP32RegClassID = 30,
  X86_VK32WMRegClassID = 31,
  X86_GR32_NOAX_and_GR32_NOREX_NOSPRegClassID = 32,
  X86_GR32_ABCDRegClassID = 33,
  X86_GR32_ABCD_and_GR32_NOAXRegClassID = 34,
  X86_GR32_TCRegClassID = 35,
  X86_GR32_ADRegClassID = 36,
  X86_GR32_NOAX_and_GR32_TCRegClassID = 37,
  X86_CCRRegClassID = 38,
  X86_GR32_AD_and_GR32_NOAXRegClassID = 39,
  X86_RFP64RegClassID = 40,
  X86_FR64XRegClassID = 41,
  X86_GR64RegClassID = 42,
  X86_CONTROL_REGRegClassID = 43,
  X86_FR64RegClassID = 44,
  X86_GR64_with_sub_8bitRegClassID = 45,
  X86_GR64_NOSPRegClassID = 46,
  X86_GR64_with_sub_32bit_in_GR32_NOAXRegClassID = 47,
  X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSPRegClassID = 48,
  X86_GR64_NOREXRegClassID = 49,
  X86_GR64_TCRegClassID = 50,
  X86_GR64_NOSP_and_GR64_TCRegClassID = 51,
  X86_GR64_with_sub_16bit_in_GR16_NOREXRegClassID = 52,
  X86_VK64RegClassID = 53,
  X86_VR64RegClassID = 54,
  X86_GR64_NOREX_NOSPRegClassID = 55,
  X86_GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAXRegClassID = 56,
  X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXRegClassID = 57,
  X86_VK64WMRegClassID = 58,
  X86_GR64_NOREX_and_GR64_TCRegClassID = 59,
  X86_GR64_TCW64RegClassID = 60,
  X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSPRegClassID = 61,
  X86_GR64_NOREX_NOSP_and_GR64_TCRegClassID = 62,
  X86_GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAXRegClassID = 63,
  X86_GR64_ABCDRegClassID = 64,
  X86_GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXRegClassID = 65,
  X86_GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAXRegClassID = 66,
  X86_GR64_with_sub_32bit_in_GR32_TCRegClassID = 67,
  X86_GR64_with_sub_32bit_in_GR32_ADRegClassID = 68,
  X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TCRegClassID = 69,
  X86_GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAXRegClassID = 70,
  X86_RSTRegClassID = 71,
  X86_RFP80RegClassID = 72,
  X86_VR128XRegClassID = 73,
  X86_VR128RegClassID = 74,
  X86_VR256XRegClassID = 75,
  X86_VR256RegClassID = 76,
  X86_VR512RegClassID = 77,
  X86_VR512_with_sub_xmm_in_FR32RegClassID = 78,
};

#endif // GET_REGINFO_ENUM

#ifdef GET_REGINFO_MC_DESC
#undef GET_REGINFO_MC_DESC

static const MCPhysReg X86RegDiffLists[] = {
  /* 0 */ 0, 1, 0,
  /* 3 */ 2, 1, 0,
  /* 6 */ 5, 1, 0,
  /* 9 */ 65522, 16, 1, 0,
  /* 13 */ 65522, 17, 1, 0,
  /* 17 */ 65427, 1, 0,
  /* 20 */ 65475, 1, 0,
  /* 23 */ 65520, 65522, 1, 0,
  /* 27 */ 65520, 65527, 1, 0,
  /* 31 */ 8, 2, 0,
  /* 34 */ 4, 0,
  /* 36 */ 65521, 8, 0,
  /* 39 */ 9, 0,
  /* 41 */ 13, 0,
  /* 43 */ 65535, 65519, 14, 0,
  /* 47 */ 65535, 65520, 14, 0,
  /* 51 */ 65528, 15, 0,
  /* 54 */ 2, 6, 16, 0,
  /* 58 */ 5, 6, 16, 0,
  /* 62 */ 65535, 9, 16, 0,
  /* 66 */ 2, 10, 16, 0,
  /* 70 */ 3, 10, 16, 0,
  /* 74 */ 3, 13, 16, 0,
  /* 78 */ 4, 13, 16, 0,
  /* 82 */ 65535, 14, 16, 0,
  /* 86 */ 1, 16, 16, 0,
  /* 90 */ 2, 16, 16, 0,
  /* 94 */ 17, 0,
  /* 96 */ 32, 32, 0,
  /* 99 */ 65221, 0,
  /* 101 */ 65381, 0,
  /* 103 */ 65389, 0,
  /* 105 */ 65397, 0,
  /* 107 */ 16, 65528, 65416, 0,
  /* 111 */ 65445, 0,
  /* 113 */ 65477, 0,
  /* 115 */ 65504, 65504, 0,
  /* 118 */ 65509, 0,
  /* 120 */ 120, 8, 65520, 0,
  /* 124 */ 65523, 0,
  /* 126 */ 65530, 0,
  /* 128 */ 65531, 0,
  /* 130 */ 65532, 0,
  /* 132 */ 65520, 65530, 65534, 65533, 0,
  /* 137 */ 65534, 0,
  /* 139 */ 65520, 65523, 65533, 65535, 0,
  /* 144 */ 65520, 65526, 65534, 65535, 0,
  /* 149 */ 65520, 65520, 65535, 65535, 0,
};

static const uint16_t X86SubRegIdxLists[] = {
  /* 0 */ 4, 3, 1, 0,
  /* 4 */ 4, 3, 1, 2, 0,
  /* 9 */ 4, 3, 0,
  /* 12 */ 6, 5, 0,
};

static MCRegisterDesc X86RegDesc[] = { // Descriptors
  { 5, 0, 0, 0, 0, 0 },
  { 850, 2, 90, 3, 2273, 0 },
  { 878, 2, 86, 3, 2273, 0 },
  { 996, 151, 87, 6, 0, 2 },
  { 853, 2, 78, 3, 2193, 0 },
  { 881, 2, 74, 3, 2193, 0 },
  { 907, 1, 83, 2, 544, 3 },
  { 898, 2, 82, 3, 544, 0 },
  { 1004, 141, 75, 6, 48, 2 },
  { 856, 2, 70, 3, 2081, 0 },
  { 884, 2, 66, 3, 2081, 0 },
  { 930, 2, 2, 3, 2081, 0 },
  { 1012, 146, 67, 6, 96, 2 },
  { 859, 2, 58, 3, 2049, 0 },
  { 863, 1, 63, 2, 624, 3 },
  { 890, 2, 62, 3, 624, 0 },
  { 887, 2, 54, 3, 2017, 0 },
  { 933, 2, 2, 3, 2017, 0 },
  { 1020, 134, 55, 6, 496, 2 },
  { 995, 150, 56, 5, 0, 2 },
  { 906, 24, 56, 1, 544, 3 },
  { 1003, 140, 56, 5, 323, 2 },
  { 1011, 145, 56, 5, 323, 2 },
  { 862, 28, 56, 1, 624, 3 },
  { 1019, 133, 56, 5, 496, 2 },
  { 942, 2, 2, 3, 1985, 0 },
  { 914, 37, 52, 10, 1985, 5 },
  { 1027, 2, 2, 3, 1985, 0 },
  { 936, 2, 2, 3, 1985, 0 },
  { 870, 10, 45, 1, 1985, 3 },
  { 922, 14, 45, 1, 1985, 3 },
  { 990, 2, 2, 3, 1985, 0 },
  { 939, 2, 2, 3, 1985, 0 },
  { 946, 2, 2, 3, 1985, 0 },
  { 915, 2, 51, 3, 656, 0 },
  { 999, 149, 2, 4, 0, 2 },
  { 910, 23, 2, 0, 544, 3 },
  { 1007, 139, 2, 4, 275, 2 },
  { 1015, 144, 2, 4, 275, 2 },
  { 866, 27, 2, 0, 624, 3 },
  { 1023, 132, 2, 4, 496, 2 },
  { 918, 36, 2, 9, 1592, 5 },
  { 1031, 2, 2, 3, 1592, 0 },
  { 874, 9, 2, 0, 1889, 3 },
  { 926, 13, 2, 0, 1889, 3 },
  { 871, 1, 48, 2, 896, 3 },
  { 894, 2, 47, 3, 896, 0 },
  { 923, 1, 44, 2, 1504, 3 },
  { 902, 2, 43, 3, 1504, 0 },
  { 949, 2, 2, 3, 1889, 0 },
  { 86, 2, 2, 3, 1889, 0 },
  { 184, 2, 2, 3, 1889, 0 },
  { 264, 2, 2, 3, 1889, 0 },
  { 344, 2, 2, 3, 1889, 0 },
  { 424, 2, 2, 3, 1889, 0 },
  { 504, 2, 2, 3, 1889, 0 },
  { 574, 2, 2, 3, 1889, 0 },
  { 644, 2, 2, 3, 1889, 0 },
  { 707, 2, 2, 3, 1889, 0 },
  { 766, 2, 2, 3, 1889, 0 },
  { 18, 2, 2, 3, 1889, 0 },
  { 116, 2, 2, 3, 1889, 0 },
  { 214, 2, 2, 3, 1889, 0 },
  { 294, 2, 2, 3, 1889, 0 },
  { 374, 2, 2, 3, 1889, 0 },
  { 454, 2, 2, 3, 1889, 0 },
  { 90, 2, 2, 3, 1889, 0 },
  { 188, 2, 2, 3, 1889, 0 },
  { 268, 2, 2, 3, 1889, 0 },
  { 348, 2, 2, 3, 1889, 0 },
  { 428, 2, 2, 3, 1889, 0 },
  { 508, 2, 2, 3, 1889, 0 },
  { 578, 2, 2, 3, 1889, 0 },
  { 648, 2, 2, 3, 1889, 0 },
  { 711, 2, 2, 3, 1889, 0 },
  { 770, 2, 2, 3, 1889, 0 },
  { 23, 2, 2, 3, 1889, 0 },
  { 121, 2, 2, 3, 1889, 0 },
  { 219, 2, 2, 3, 1889, 0 },
  { 299, 2, 2, 3, 1889, 0 },
  { 379, 2, 2, 3, 1889, 0 },
  { 459, 2, 2, 3, 1889, 0 },
  { 82, 2, 2, 3, 1889, 0 },
  { 180, 2, 2, 3, 1889, 0 },
  { 260, 2, 2, 3, 1889, 0 },
  { 340, 2, 2, 3, 1889, 0 },
  { 420, 2, 2, 3, 1889, 0 },
  { 500, 2, 2, 3, 1889, 0 },
  { 570, 2, 2, 3, 1889, 0 },
  { 640, 2, 2, 3, 1889, 0 },
  { 64, 2, 2, 3, 1889, 0 },
  { 162, 2, 2, 3, 1889, 0 },
  { 242, 2, 2, 3, 1889, 0 },
  { 322, 2, 2, 3, 1889, 0 },
  { 402, 2, 2, 3, 1889, 0 },
  { 482, 2, 2, 3, 1889, 0 },
  { 552, 2, 2, 3, 1889, 0 },
  { 622, 2, 2, 3, 1889, 0 },
  { 68, 2, 2, 3, 1889, 0 },
  { 166, 2, 2, 3, 1889, 0 },
  { 246, 2, 2, 3, 1889, 0 },
  { 326, 2, 2, 3, 1889, 0 },
  { 406, 2, 2, 3, 1889, 0 },
  { 486, 2, 2, 3, 1889, 0 },
  { 556, 2, 2, 3, 1889, 0 },
  { 626, 2, 2, 3, 1889, 0 },
  { 708, 120, 2, 0, 1889, 3 },
  { 767, 120, 2, 0, 1889, 3 },
  { 19, 120, 2, 0, 1889, 3 },
  { 117, 120, 2, 0, 1889, 3 },
  { 215, 120, 2, 0, 1889, 3 },
  { 295, 120, 2, 0, 1889, 3 },
  { 375, 120, 2, 0, 1889, 3 },
  { 455, 120, 2, 0, 1889, 3 },
  { 94, 2, 2, 3, 1889, 0 },
  { 192, 2, 2, 3, 1889, 0 },
  { 272, 2, 2, 3, 1889, 0 },
  { 352, 2, 2, 3, 1889, 0 },
  { 432, 2, 2, 3, 1889, 0 },
  { 512, 2, 2, 3, 1889, 0 },
  { 582, 2, 2, 3, 1889, 0 },
  { 652, 2, 2, 3, 1889, 0 },
  { 67, 2, 96, 3, 1889, 0 },
  { 165, 2, 96, 3, 1889, 0 },
  { 245, 2, 96, 3, 1889, 0 },
  { 325, 2, 96, 3, 1889, 0 },
  { 405, 2, 96, 3, 1889, 0 },
  { 485, 2, 96, 3, 1889, 0 },
  { 555, 2, 96, 3, 1889, 0 },
  { 625, 2, 96, 3, 1889, 0 },
  { 692, 2, 96, 3, 1889, 0 },
  { 751, 2, 96, 3, 1889, 0 },
  { 0, 2, 96, 3, 1889, 0 },
  { 98, 2, 96, 3, 1889, 0 },
  { 196, 2, 96, 3, 1889, 0 },
  { 276, 2, 96, 3, 1889, 0 },
  { 356, 2, 96, 3, 1889, 0 },
  { 436, 2, 96, 3, 1889, 0 },
  { 516, 2, 96, 3, 1889, 0 },
  { 586, 2, 96, 3, 1889, 0 },
  { 656, 2, 96, 3, 1889, 0 },
  { 715, 2, 96, 3, 1889, 0 },
  { 28, 2, 96, 3, 1889, 0 },
  { 126, 2, 96, 3, 1889, 0 },
  { 224, 2, 96, 3, 1889, 0 },
  { 304, 2, 96, 3, 1889, 0 },
  { 384, 2, 96, 3, 1889, 0 },
  { 464, 2, 96, 3, 1889, 0 },
  { 534, 2, 96, 3, 1889, 0 },
  { 604, 2, 96, 3, 1889, 0 },
  { 674, 2, 96, 3, 1889, 0 },
  { 733, 2, 96, 3, 1889, 0 },
  { 46, 2, 96, 3, 1889, 0 },
  { 144, 2, 96, 3, 1889, 0 },
  { 72, 116, 97, 13, 1809, 7 },
  { 170, 116, 97, 13, 1809, 7 },
  { 250, 116, 97, 13, 1809, 7 },
  { 330, 116, 97, 13, 1809, 7 },
  { 410, 116, 97, 13, 1809, 7 },
  { 490, 116, 97, 13, 1809, 7 },
  { 560, 116, 97, 13, 1809, 7 },
  { 630, 116, 97, 13, 1809, 7 },
  { 697, 116, 97, 13, 1809, 7 },
  { 756, 116, 97, 13, 1809, 7 },
  { 6, 116, 97, 13, 1809, 7 },
  { 104, 116, 97, 13, 1809, 7 },
  { 202, 116, 97, 13, 1809, 7 },
  { 282, 116, 97, 13, 1809, 7 },
  { 362, 116, 97, 13, 1809, 7 },
  { 442, 116, 97, 13, 1809, 7 },
  { 522, 116, 97, 13, 1809, 7 },
  { 592, 116, 97, 13, 1809, 7 },
  { 662, 116, 97, 13, 1809, 7 },
  { 721, 116, 97, 13, 1809, 7 },
  { 34, 116, 97, 13, 1809, 7 },
  { 132, 116, 97, 13, 1809, 7 },
  { 230, 116, 97, 13, 1809, 7 },
  { 310, 116, 97, 13, 1809, 7 },
  { 390, 116, 97, 13, 1809, 7 },
  { 470, 116, 97, 13, 1809, 7 },
  { 540, 116, 97, 13, 1809, 7 },
  { 610, 116, 97, 13, 1809, 7 },
  { 680, 116, 97, 13, 1809, 7 },
  { 739, 116, 97, 13, 1809, 7 },
  { 52, 116, 97, 13, 1809, 7 },
  { 150, 116, 97, 13, 1809, 7 },
  { 77, 115, 2, 12, 1777, 7 },
  { 175, 115, 2, 12, 1777, 7 },
  { 255, 115, 2, 12, 1777, 7 },
  { 335, 115, 2, 12, 1777, 7 },
  { 415, 115, 2, 12, 1777, 7 },
  { 495, 115, 2, 12, 1777, 7 },
  { 565, 115, 2, 12, 1777, 7 },
  { 635, 115, 2, 12, 1777, 7 },
  { 702, 115, 2, 12, 1777, 7 },
  { 761, 115, 2, 12, 1777, 7 },
  { 12, 115, 2, 12, 1777, 7 },
  { 110, 115, 2, 12, 1777, 7 },
  { 208, 115, 2, 12, 1777, 7 },
  { 288, 115, 2, 12, 1777, 7 },
  { 368, 115, 2, 12, 1777, 7 },
  { 448, 115, 2, 12, 1777, 7 },
  { 528, 115, 2, 12, 1777, 7 },
  { 598, 115, 2, 12, 1777, 7 },
  { 668, 115, 2, 12, 1777, 7 },
  { 727, 115, 2, 12, 1777, 7 },
  { 40, 115, 2, 12, 1777, 7 },
  { 138, 115, 2, 12, 1777, 7 },
  { 236, 115, 2, 12, 1777, 7 },
  { 316, 115, 2, 12, 1777, 7 },
  { 396, 115, 2, 12, 1777, 7 },
  { 476, 115, 2, 12, 1777, 7 },
  { 546, 115, 2, 12, 1777, 7 },
  { 616, 115, 2, 12, 1777, 7 },
  { 686, 115, 2, 12, 1777, 7 },
  { 745, 115, 2, 12, 1777, 7 },
  { 58, 115, 2, 12, 1777, 7 },
  { 156, 115, 2, 12, 1777, 7 },
  { 804, 2, 107, 3, 1681, 0 },
  { 808, 2, 107, 3, 1681, 0 },
  { 774, 2, 107, 3, 1681, 0 },
  { 779, 2, 107, 3, 1681, 0 },
  { 784, 2, 107, 3, 1681, 0 },
  { 789, 2, 107, 3, 1681, 0 },
  { 794, 2, 107, 3, 1681, 0 },
  { 799, 2, 107, 3, 1681, 0 },
  { 842, 121, 109, 1, 1649, 3 },
  { 846, 121, 109, 1, 1649, 3 },
  { 812, 121, 109, 1, 1649, 3 },
  { 817, 121, 109, 1, 1649, 3 },
  { 822, 121, 109, 1, 1649, 3 },
  { 827, 121, 109, 1, 1649, 3 },
  { 832, 121, 109, 1, 1649, 3 },
  { 837, 121, 109, 1, 1649, 3 },
  { 982, 122, 108, 2, 1617, 3 },
  { 986, 122, 108, 2, 1617, 3 },
  { 952, 122, 108, 2, 1617, 3 },
  { 957, 122, 108, 2, 1617, 3 },
  { 962, 122, 108, 2, 1617, 3 },
  { 967, 122, 108, 2, 1617, 3 },
  { 972, 122, 108, 2, 1617, 3 },
  { 977, 122, 108, 2, 1617, 3 },
};

  // GR8 Register Class...
  static const MCPhysReg GR8[] = {
    X86_AL, X86_CL, X86_DL, X86_AH, X86_CH, X86_DH, X86_BL, X86_BH, X86_SIL, X86_DIL, X86_BPL, X86_SPL, X86_R8B, X86_R9B, X86_R10B, X86_R11B, X86_R14B, X86_R15B, X86_R12B, X86_R13B, 
  };

  // GR8 Bit set.
  static uint8_t GR8Bits[] = {
    0xb6, 0xa6, 0x01, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR8_NOREX Register Class...
  static const MCPhysReg GR8_NOREX[] = {
    X86_AL, X86_CL, X86_DL, X86_AH, X86_CH, X86_DH, X86_BL, X86_BH, 
  };

  // GR8_NOREX Bit set.
  static const uint8_t GR8_NOREXBits[] = {
    0x36, 0x26, 0x01, 
  };

  // VK1 Register Class...
  static const MCPhysReg VK1[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK1 Bit set.
  static uint8_t VK1Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VK2 Register Class...
  static const MCPhysReg VK2[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK2 Bit set.
  static uint8_t VK2Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VK4 Register Class...
  static const MCPhysReg VK4[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK4 Bit set.
  static uint8_t VK4Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VK8 Register Class...
  static const MCPhysReg VK8[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK8 Bit set.
  static uint8_t VK8Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VK1WM Register Class...
  static const MCPhysReg VK1WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK1WM Bit set.
  static uint8_t VK1WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // VK2WM Register Class...
  static const MCPhysReg VK2WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK2WM Bit set.
  static uint8_t VK2WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // VK4WM Register Class...
  static const MCPhysReg VK4WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK4WM Bit set.
  static uint8_t VK4WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // VK8WM Register Class...
  static const MCPhysReg VK8WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK8WM Bit set.
  static uint8_t VK8WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // GR8_ABCD_H Register Class...
  static MCPhysReg GR8_ABCD_H[] = {
    X86_AH, X86_CH, X86_DH, X86_BH, 
  };

  // GR8_ABCD_H Bit set.
  static uint8_t GR8_ABCD_HBits[] = {
    0x12, 0x22, 
  };

  // GR8_ABCD_L Register Class...
  static MCPhysReg GR8_ABCD_L[] = {
    X86_AL, X86_CL, X86_DL, X86_BL, 
  };

  // GR8_ABCD_L Bit set.
  static uint8_t GR8_ABCD_LBits[] = {
    0x24, 0x04, 0x01, 
  };

  // GR16 Register Class...
  static MCPhysReg GR16[] = {
    X86_AX, X86_CX, X86_DX, X86_SI, X86_DI, X86_BX, X86_BP, X86_SP, X86_R8W, X86_R9W, X86_R10W, X86_R11W, X86_R14W, X86_R15W, X86_R12W, X86_R13W, 
  };

  // GR16 Bit set.
  static uint8_t GR16Bits[] = {
    0x48, 0x51, 0x04, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR16_NOREX Register Class...
  static MCPhysReg GR16_NOREX[] = {
    X86_AX, X86_CX, X86_DX, X86_SI, X86_DI, X86_BX, X86_BP, X86_SP, 
  };

  // GR16_NOREX Bit set.
  static uint8_t GR16_NOREXBits[] = {
    0x48, 0x51, 0x04, 0x00, 0x00, 0xa0, 
  };

  // VK16 Register Class...
  static MCPhysReg VK16[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK16 Bit set.
  static uint8_t VK16Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VK16WM Register Class...
  static MCPhysReg VK16WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK16WM Bit set.
  static uint8_t VK16WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // SEGMENT_REG Register Class...
  static const MCPhysReg SEGMENT_REG[] = {
    X86_CS, X86_DS, X86_SS, X86_ES, X86_FS, X86_GS, 
  };

  // SEGMENT_REG Bit set.
  static const uint8_t SEGMENT_REGBits[] = {
    0x00, 0x08, 0x02, 0x10, 0x03, 0x00, 0x02, 
  };

  // GR16_ABCD Register Class...
  static const MCPhysReg GR16_ABCD[] = {
    X86_AX, X86_CX, X86_DX, X86_BX, 
  };

  // GR16_ABCD Bit set.
  static const uint8_t GR16_ABCDBits[] = {
    0x08, 0x11, 0x04, 
  };

  // FPCCR Register Class...
  static const MCPhysReg FPCCR[] = {
    X86_FPSW, 
  };

  // FPCCR Bit set.
  static const uint8_t FPCCRBits[] = {
    0x00, 0x00, 0x00, 0x80, 
  };

  // FR32X Register Class...
  static const MCPhysReg FR32X[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, X86_XMM16, X86_XMM17, X86_XMM18, X86_XMM19, X86_XMM20, X86_XMM21, X86_XMM22, X86_XMM23, X86_XMM24, X86_XMM25, X86_XMM26, X86_XMM27, X86_XMM28, X86_XMM29, X86_XMM30, X86_XMM31, 
  };

  // FR32X Bit set.
  static uint8_t FR32XBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // FR32 Register Class...
  static const MCPhysReg FR32[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, 
  };

  // FR32 Bit set.
  static uint8_t FR32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // GR32 Register Class...
  static const MCPhysReg GR32[] = {
    X86_EAX, X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_ESP, X86_R8D, X86_R9D, X86_R10D, X86_R11D, X86_R14D, X86_R15D, X86_R12D, X86_R13D, 
  };

  // GR32 Bit set.
  static uint8_t GR32Bits[] = {
    0x00, 0x00, 0xf8, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR32_NOAX Register Class...
  static const MCPhysReg GR32_NOAX[] = {
    X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_ESP, X86_R8D, X86_R9D, X86_R10D, X86_R11D, X86_R14D, X86_R15D, X86_R12D, X86_R13D, 
  };

  // GR32_NOAX Bit set.
  static uint8_t GR32_NOAXBits[] = {
    0x00, 0x00, 0xf0, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR32_NOSP Register Class...
  static const MCPhysReg GR32_NOSP[] = {
    X86_EAX, X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_R8D, X86_R9D, X86_R10D, X86_R11D, X86_R14D, X86_R15D, X86_R12D, X86_R13D, 
  };

  // GR32_NOSP Bit set.
  static uint8_t GR32_NOSPBits[] = {
    0x00, 0x00, 0xf8, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR32_NOAX_and_GR32_NOSP Register Class...
  static const MCPhysReg GR32_NOAX_and_GR32_NOSP[] = {
    X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_R8D, X86_R9D, X86_R10D, X86_R11D, X86_R14D, X86_R15D, X86_R12D, X86_R13D, 
  };

  // GR32_NOAX_and_GR32_NOSP Bit set.
  static uint8_t GR32_NOAX_and_GR32_NOSPBits[] = {
    0x00, 0x00, 0xf0, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // DEBUG_REG Register Class...
  static const MCPhysReg DEBUG_REG[] = {
    X86_DR0, X86_DR1, X86_DR2, X86_DR3, X86_DR4, X86_DR5, X86_DR6, X86_DR7, 
  };

  // DEBUG_REG Bit set.
  static const uint8_t DEBUG_REGBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR32_NOREX Register Class...
  static const MCPhysReg GR32_NOREX[] = {
    X86_EAX, X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_ESP, 
  };

  // GR32_NOREX Bit set.
  static const uint8_t GR32_NOREXBits[] = {
    0x00, 0x00, 0xf8, 0x61, 
  };

  // VK32 Register Class...
  static const MCPhysReg VK32[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK32 Bit set.
  static uint8_t VK32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR32_NOAX_and_GR32_NOREX Register Class...
  static const MCPhysReg GR32_NOAX_and_GR32_NOREX[] = {
    X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, X86_ESP, 
  };

  // GR32_NOAX_and_GR32_NOREX Bit set.
  static const uint8_t GR32_NOAX_and_GR32_NOREXBits[] = {
    0x00, 0x00, 0xf0, 0x61, 
  };

  // GR32_NOREX_NOSP Register Class...
  static const MCPhysReg GR32_NOREX_NOSP[] = {
    X86_EAX, X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, 
  };

  // GR32_NOREX_NOSP Bit set.
  static const uint8_t GR32_NOREX_NOSPBits[] = {
    0x00, 0x00, 0xf8, 0x21, 
  };

  // RFP32 Register Class...
  static const MCPhysReg RFP32[] = {
    X86_FP0, X86_FP1, X86_FP2, X86_FP3, X86_FP4, X86_FP5, X86_FP6, 
  };

  // RFP32 Bit set.
  static uint8_t RFP32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x01, 
  };

  // VK32WM Register Class...
  static const MCPhysReg VK32WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK32WM Bit set.
  static uint8_t VK32WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // GR32_NOAX_and_GR32_NOREX_NOSP Register Class...
  static const MCPhysReg GR32_NOAX_and_GR32_NOREX_NOSP[] = {
    X86_ECX, X86_EDX, X86_ESI, X86_EDI, X86_EBX, X86_EBP, 
  };

  // GR32_NOAX_and_GR32_NOREX_NOSP Bit set.
  static const uint8_t GR32_NOAX_and_GR32_NOREX_NOSPBits[] = {
    0x00, 0x00, 0xf0, 0x21, 
  };

  // GR32_ABCD Register Class...
  static const MCPhysReg GR32_ABCD[] = {
    X86_EAX, X86_ECX, X86_EDX, X86_EBX, 
  };

  // GR32_ABCD Bit set.
  static const uint8_t GR32_ABCDBits[] = {
    0x00, 0x00, 0x68, 0x01, 
  };

  // GR32_ABCD_and_GR32_NOAX Register Class...
  static const MCPhysReg GR32_ABCD_and_GR32_NOAX[] = {
    X86_ECX, X86_EDX, X86_EBX, 
  };

  // GR32_ABCD_and_GR32_NOAX Bit set.
  static const uint8_t GR32_ABCD_and_GR32_NOAXBits[] = {
    0x00, 0x00, 0x60, 0x01, 
  };

  // GR32_TC Register Class...
  static const MCPhysReg GR32_TC[] = {
    X86_EAX, X86_ECX, X86_EDX, 
  };

  // GR32_TC Bit set.
  static const uint8_t GR32_TCBits[] = {
    0x00, 0x00, 0x48, 0x01, 
  };

  // GR32_AD Register Class...
  static const MCPhysReg GR32_AD[] = {
    X86_EAX, X86_EDX, 
  };

  // GR32_AD Bit set.
  static const uint8_t GR32_ADBits[] = {
    0x00, 0x00, 0x08, 0x01, 
  };

  // GR32_NOAX_and_GR32_TC Register Class...
  static const MCPhysReg GR32_NOAX_and_GR32_TC[] = {
    X86_ECX, X86_EDX, 
  };

  // GR32_NOAX_and_GR32_TC Bit set.
  static const uint8_t GR32_NOAX_and_GR32_TCBits[] = {
    0x00, 0x00, 0x40, 0x01, 
  };

  // CCR Register Class...
  static const MCPhysReg CCR[] = {
    X86_EFLAGS, 
  };

  // CCR Bit set.
  static const uint8_t CCRBits[] = {
    0x00, 0x00, 0x00, 0x02, 
  };

  // GR32_AD_and_GR32_NOAX Register Class...
  static const MCPhysReg GR32_AD_and_GR32_NOAX[] = {
    X86_EDX, 
  };

  // GR32_AD_and_GR32_NOAX Bit set.
  static const uint8_t GR32_AD_and_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x01, 
  };

  // RFP64 Register Class...
  static const MCPhysReg RFP64[] = {
    X86_FP0, X86_FP1, X86_FP2, X86_FP3, X86_FP4, X86_FP5, X86_FP6, 
  };

  // RFP64 Bit set.
  static uint8_t RFP64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x01, 
  };

  // FR64X Register Class...
  static const MCPhysReg FR64X[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, X86_XMM16, X86_XMM17, X86_XMM18, X86_XMM19, X86_XMM20, X86_XMM21, X86_XMM22, X86_XMM23, X86_XMM24, X86_XMM25, X86_XMM26, X86_XMM27, X86_XMM28, X86_XMM29, X86_XMM30, X86_XMM31, 
  };

  // FR64X Bit set.
  static uint8_t FR64XBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // GR64 Register Class...
  static const MCPhysReg GR64[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_R11, X86_RBX, X86_R14, X86_R15, X86_R12, X86_R13, X86_RBP, X86_RSP, X86_RIP, 
  };

  // GR64 Bit set.
  static uint8_t GR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // CONTROL_REG Register Class...
  static const MCPhysReg CONTROL_REG[] = {
    X86_CR0, X86_CR1, X86_CR2, X86_CR3, X86_CR4, X86_CR5, X86_CR6, X86_CR7, X86_CR8, X86_CR9, X86_CR10, X86_CR11, X86_CR12, X86_CR13, X86_CR14, X86_CR15, 
  };

  // CONTROL_REG Bit set.
  static const uint8_t CONTROL_REGBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // FR64 Register Class...
  static const MCPhysReg FR64[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, 
  };

  // FR64 Bit set.
  static uint8_t FR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // GR64_with_sub_8bit Register Class...
  static const MCPhysReg GR64_with_sub_8bit[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_R11, X86_RBX, X86_R14, X86_R15, X86_R12, X86_R13, X86_RBP, X86_RSP, 
  };

  // GR64_with_sub_8bit Bit set.
  static uint8_t GR64_with_sub_8bitBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR64_NOSP Register Class...
  static const MCPhysReg GR64_NOSP[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_R11, X86_RBX, X86_R14, X86_R15, X86_R12, X86_R13, X86_RBP, 
  };

  // GR64_NOSP Bit set.
  static uint8_t GR64_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_NOAX[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_R11, X86_RBX, X86_R14, X86_R15, X86_R12, X86_R13, X86_RBP, X86_RSP, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX Bit set.
  static uint8_t GR64_with_sub_32bit_in_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf0, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSP Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSP[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_R11, X86_RBX, X86_R14, X86_R15, X86_R12, X86_R13, X86_RBP, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSP Bit set.
  static uint8_t GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR64_NOREX Register Class...
  static const MCPhysReg GR64_NOREX[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RBX, X86_RBP, X86_RSP, X86_RIP, 
  };

  // GR64_NOREX Bit set.
  static const uint8_t GR64_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x1b, 
  };

  // GR64_TC Register Class...
  static const MCPhysReg GR64_TC[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R11, X86_RIP, 
  };

  // GR64_TC Bit set.
  static uint8_t GR64_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc8, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 
  };

  // GR64_NOSP_and_GR64_TC Register Class...
  static const MCPhysReg GR64_NOSP_and_GR64_TC[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R11, 
  };

  // GR64_NOSP_and_GR64_TC Bit set.
  static uint8_t GR64_NOSP_and_GR64_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc8, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 
  };

  // GR64_with_sub_16bit_in_GR16_NOREX Register Class...
  static const MCPhysReg GR64_with_sub_16bit_in_GR16_NOREX[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RBX, X86_RBP, X86_RSP, 
  };

  // GR64_with_sub_16bit_in_GR16_NOREX Bit set.
  static const uint8_t GR64_with_sub_16bit_in_GR16_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x19, 
  };

  // VK64 Register Class...
  static const MCPhysReg VK64[] = {
    X86_K0, X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK64 Bit set.
  static uint8_t VK64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // VR64 Register Class...
  static const MCPhysReg VR64[] = {
    X86_MM0, X86_MM1, X86_MM2, X86_MM3, X86_MM4, X86_MM5, X86_MM6, X86_MM7, 
  };

  // VR64 Bit set.
  static uint8_t VR64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // GR64_NOREX_NOSP Register Class...
  static const MCPhysReg GR64_NOREX_NOSP[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RBX, X86_RBP, 
  };

  // GR64_NOREX_NOSP Bit set.
  static const uint8_t GR64_NOREX_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x09, 
  };

  // GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX Register Class...
  static const MCPhysReg GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R11, 
  };

  // GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX Bit set.
  static uint8_t GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RBX, X86_RBP, X86_RSP, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf0, 0x19, 
  };

  // VK64WM Register Class...
  static const MCPhysReg VK64WM[] = {
    X86_K1, X86_K2, X86_K3, X86_K4, X86_K5, X86_K6, X86_K7, 
  };

  // VK64WM Bit set.
  static uint8_t VK64WMBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x03, 
  };

  // GR64_NOREX_and_GR64_TC Register Class...
  static const MCPhysReg GR64_NOREX_and_GR64_TC[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RIP, 
  };

  // GR64_NOREX_and_GR64_TC Bit set.
  static const uint8_t GR64_NOREX_and_GR64_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc8, 0x0b, 
  };

  // GR64_TCW64 Register Class...
  static const MCPhysReg GR64_TCW64[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_R8, X86_R9, X86_R11, 
  };

  // GR64_TCW64 Bit set.
  static uint8_t GR64_TCW64Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSP Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSP[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, X86_RBX, X86_RBP, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSP Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSPBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xf0, 0x09, 
  };

  // GR64_NOREX_NOSP_and_GR64_TC Register Class...
  static const MCPhysReg GR64_NOREX_NOSP_and_GR64_TC[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RSI, X86_RDI, 
  };

  // GR64_NOREX_NOSP_and_GR64_TC Bit set.
  static const uint8_t GR64_NOREX_NOSP_and_GR64_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc8, 0x09, 
  };

  // GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAX Register Class...
  static const MCPhysReg GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAX[] = {
    X86_RCX, X86_RDX, X86_R8, X86_R9, X86_R11, 
  };

  // GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAX Bit set.
  static uint8_t GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 
  };

  // GR64_ABCD Register Class...
  static const MCPhysReg GR64_ABCD[] = {
    X86_RAX, X86_RCX, X86_RDX, X86_RBX, 
  };

  // GR64_ABCD Bit set.
  static const uint8_t GR64_ABCDBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x68, 0x01, 
  };

  // GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX Register Class...
  static const MCPhysReg GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX[] = {
    X86_RCX, X86_RDX, X86_RSI, X86_RDI, 
  };

  // GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX Bit set.
  static const uint8_t GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0xc0, 0x09, 
  };

  // GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAX Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAX[] = {
    X86_RCX, X86_RDX, X86_RBX, 
  };

  // GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAX Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 
  };

  // GR64_with_sub_32bit_in_GR32_TC Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_TC[] = {
    X86_RAX, X86_RCX, X86_RDX, 
  };

  // GR64_with_sub_32bit_in_GR32_TC Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x48, 0x01, 
  };

  // GR64_with_sub_32bit_in_GR32_AD Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_AD[] = {
    X86_RAX, X86_RDX, 
  };

  // GR64_with_sub_32bit_in_GR32_AD Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_ADBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x08, 0x01, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TC Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TC[] = {
    X86_RCX, X86_RDX, 
  };

  // GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TC Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 
  };

  // GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAX Register Class...
  static const MCPhysReg GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAX[] = {
    X86_RDX, 
  };

  // GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAX Bit set.
  static const uint8_t GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAXBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
  };

  // RST Register Class...
  static const MCPhysReg RST[] = {
    X86_ST0, X86_ST1, X86_ST2, X86_ST3, X86_ST4, X86_ST5, X86_ST6, X86_ST7, 
  };

  // RST Bit set.
  static uint8_t RSTBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 
  };

  // RFP80 Register Class...
  static const MCPhysReg RFP80[] = {
    X86_FP0, X86_FP1, X86_FP2, X86_FP3, X86_FP4, X86_FP5, X86_FP6, 
  };

  // RFP80 Bit set.
  static uint8_t RFP80Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x01, 
  };

  // VR128X Register Class...
  static const MCPhysReg VR128X[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, X86_XMM16, X86_XMM17, X86_XMM18, X86_XMM19, X86_XMM20, X86_XMM21, X86_XMM22, X86_XMM23, X86_XMM24, X86_XMM25, X86_XMM26, X86_XMM27, X86_XMM28, X86_XMM29, X86_XMM30, X86_XMM31, 
  };

  // VR128X Bit set.
  static uint8_t VR128XBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // VR128 Register Class...
  static const MCPhysReg VR128[] = {
    X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3, X86_XMM4, X86_XMM5, X86_XMM6, X86_XMM7, X86_XMM8, X86_XMM9, X86_XMM10, X86_XMM11, X86_XMM12, X86_XMM13, X86_XMM14, X86_XMM15, 
  };

  // VR128 Bit set.
  static uint8_t VR128Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // VR256X Register Class...
  static const MCPhysReg VR256X[] = {
    X86_YMM0, X86_YMM1, X86_YMM2, X86_YMM3, X86_YMM4, X86_YMM5, X86_YMM6, X86_YMM7, X86_YMM8, X86_YMM9, X86_YMM10, X86_YMM11, X86_YMM12, X86_YMM13, X86_YMM14, X86_YMM15, X86_YMM16, X86_YMM17, X86_YMM18, X86_YMM19, X86_YMM20, X86_YMM21, X86_YMM22, X86_YMM23, X86_YMM24, X86_YMM25, X86_YMM26, X86_YMM27, X86_YMM28, X86_YMM29, X86_YMM30, X86_YMM31, 
  };

  // VR256X Bit set.
  static uint8_t VR256XBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // VR256 Register Class...
  static const MCPhysReg VR256[] = {
    X86_YMM0, X86_YMM1, X86_YMM2, X86_YMM3, X86_YMM4, X86_YMM5, X86_YMM6, X86_YMM7, X86_YMM8, X86_YMM9, X86_YMM10, X86_YMM11, X86_YMM12, X86_YMM13, X86_YMM14, X86_YMM15, 
  };

  // VR256 Bit set.
  static uint8_t VR256Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

  // VR512 Register Class...
  static const MCPhysReg VR512[] = {
    X86_ZMM0, X86_ZMM1, X86_ZMM2, X86_ZMM3, X86_ZMM4, X86_ZMM5, X86_ZMM6, X86_ZMM7, X86_ZMM8, X86_ZMM9, X86_ZMM10, X86_ZMM11, X86_ZMM12, X86_ZMM13, X86_ZMM14, X86_ZMM15, X86_ZMM16, X86_ZMM17, X86_ZMM18, X86_ZMM19, X86_ZMM20, X86_ZMM21, X86_ZMM22, X86_ZMM23, X86_ZMM24, X86_ZMM25, X86_ZMM26, X86_ZMM27, X86_ZMM28, X86_ZMM29, X86_ZMM30, X86_ZMM31, 
  };

  // VR512 Bit set.
  static uint8_t VR512Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x03, 
  };

  // VR512_with_sub_xmm_in_FR32 Register Class...
  static const MCPhysReg VR512_with_sub_xmm_in_FR32[] = {
    X86_ZMM0, X86_ZMM1, X86_ZMM2, X86_ZMM3, X86_ZMM4, X86_ZMM5, X86_ZMM6, X86_ZMM7, X86_ZMM8, X86_ZMM9, X86_ZMM10, X86_ZMM11, X86_ZMM12, X86_ZMM13, X86_ZMM14, X86_ZMM15, 
  };

  // VR512_with_sub_xmm_in_FR32 Bit set.
  static uint8_t VR512_with_sub_xmm_in_FR32Bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x03, 
  };

static MCRegisterClass X86MCRegisterClasses[] = {
  { GR8, GR8Bits, 130, 20, sizeof(GR8Bits), X86_GR8RegClassID, 1, 1, 1, 1 },
  { GR8_NOREX, GR8_NOREXBits, 897, 8, sizeof(GR8_NOREXBits), X86_GR8_NOREXRegClassID, 1, 1, 1, 1 },
  { VK1, VK1Bits, 6, 8, sizeof(VK1Bits), X86_VK1RegClassID, 1, 1, 1, 1 },
  { VK2, VK2Bits, 59, 8, sizeof(VK2Bits), X86_VK2RegClassID, 1, 1, 1, 1 },
  { VK4, VK4Bits, 100, 8, sizeof(VK4Bits), X86_VK4RegClassID, 1, 1, 1, 1 },
  { VK8, VK8Bits, 126, 8, sizeof(VK8Bits), X86_VK8RegClassID, 1, 1, 1, 1 },
  { VK1WM, VK1WMBits, 400, 7, sizeof(VK1WMBits), X86_VK1WMRegClassID, 1, 1, 1, 1 },
  { VK2WM, VK2WMBits, 413, 7, sizeof(VK2WMBits), X86_VK2WMRegClassID, 1, 1, 1, 1 },
  { VK4WM, VK4WMBits, 426, 7, sizeof(VK4WMBits), X86_VK4WMRegClassID, 1, 1, 1, 1 },
  { VK8WM, VK8WMBits, 439, 7, sizeof(VK8WMBits), X86_VK8WMRegClassID, 1, 1, 1, 1 },
  { GR8_ABCD_H, GR8_ABCD_HBits, 378, 4, sizeof(GR8_ABCD_HBits), X86_GR8_ABCD_HRegClassID, 1, 1, 1, 1 },
  { GR8_ABCD_L, GR8_ABCD_LBits, 389, 4, sizeof(GR8_ABCD_LBits), X86_GR8_ABCD_LRegClassID, 1, 1, 1, 1 },
  { GR16, GR16Bits, 109, 16, sizeof(GR16Bits), X86_GR16RegClassID, 2, 2, 1, 1 },
  { GR16_NOREX, GR16_NOREXBits, 886, 8, sizeof(GR16_NOREXBits), X86_GR16_NOREXRegClassID, 2, 2, 1, 1 },
  { VK16, VK16Bits, 104, 8, sizeof(VK16Bits), X86_VK16RegClassID, 2, 2, 1, 1 },
  { VK16WM, VK16WMBits, 432, 7, sizeof(VK16WMBits), X86_VK16WMRegClassID, 2, 2, 1, 1 },
  { SEGMENT_REG, SEGMENT_REGBits, 366, 6, sizeof(SEGMENT_REGBits), X86_SEGMENT_REGRegClassID, 2, 2, 1, 1 },
  { GR16_ABCD, GR16_ABCDBits, 334, 4, sizeof(GR16_ABCDBits), X86_GR16_ABCDRegClassID, 2, 2, 1, 1 },
  { FPCCR, FPCCRBits, 571, 1, sizeof(FPCCRBits), X86_FPCCRRegClassID, 2, 2, -1, 0 },
  { FR32X, FR32XBits, 581, 32, sizeof(FR32XBits), X86_FR32XRegClassID, 4, 4, 1, 1 },
  { FR32, FR32Bits, 49, 16, sizeof(FR32Bits), X86_FR32RegClassID, 4, 4, 1, 1 },
  { GR32, GR32Bits, 54, 16, sizeof(GR32Bits), X86_GR32RegClassID, 4, 4, 1, 1 },
  { GR32_NOAX, GR32_NOAXBits, 642, 15, sizeof(GR32_NOAXBits), X86_GR32_NOAXRegClassID, 4, 4, 1, 1 },
  { GR32_NOSP, GR32_NOSPBits, 482, 15, sizeof(GR32_NOSPBits), X86_GR32_NOSPRegClassID, 4, 4, 1, 1 },
  { GR32_NOAX_and_GR32_NOSP, GR32_NOAX_and_GR32_NOSPBits, 468, 14, sizeof(GR32_NOAX_and_GR32_NOSPBits), X86_GR32_NOAX_and_GR32_NOSPRegClassID, 4, 4, 1, 1 },
  { DEBUG_REG, DEBUG_REGBits, 344, 8, sizeof(DEBUG_REGBits), X86_DEBUG_REGRegClassID, 4, 4, 1, 1 },
  { GR32_NOREX, GR32_NOREXBits, 841, 8, sizeof(GR32_NOREXBits), X86_GR32_NOREXRegClassID, 4, 4, 1, 1 },
  { VK32, VK32Bits, 16, 8, sizeof(VK32Bits), X86_VK32RegClassID, 4, 4, 1, 1 },
  { GR32_NOAX_and_GR32_NOREX, GR32_NOAX_and_GR32_NOREXBits, 827, 7, sizeof(GR32_NOAX_and_GR32_NOREXBits), X86_GR32_NOAX_and_GR32_NOREXRegClassID, 4, 4, 1, 1 },
  { GR32_NOREX_NOSP, GR32_NOREX_NOSPBits, 539, 7, sizeof(GR32_NOREX_NOSPBits), X86_GR32_NOREX_NOSPRegClassID, 4, 4, 1, 1 },
  { RFP32, RFP32Bits, 21, 7, sizeof(RFP32Bits), X86_RFP32RegClassID, 4, 4, 1, 1 },
  { VK32WM, VK32WMBits, 406, 7, sizeof(VK32WMBits), X86_VK32WMRegClassID, 4, 4, 1, 1 },
  { GR32_NOAX_and_GR32_NOREX_NOSP, GR32_NOAX_and_GR32_NOREX_NOSPBits, 525, 6, sizeof(GR32_NOAX_and_GR32_NOREX_NOSPBits), X86_GR32_NOAX_and_GR32_NOREX_NOSPRegClassID, 4, 4, 1, 1 },
  { GR32_ABCD, GR32_ABCDBits, 314, 4, sizeof(GR32_ABCDBits), X86_GR32_ABCDRegClassID, 4, 4, 1, 1 },
  { GR32_ABCD_and_GR32_NOAX, GR32_ABCD_and_GR32_NOAXBits, 675, 3, sizeof(GR32_ABCD_and_GR32_NOAXBits), X86_GR32_ABCD_and_GR32_NOAXRegClassID, 4, 4, 1, 1 },
  { GR32_TC, GR32_TCBits, 171, 3, sizeof(GR32_TCBits), X86_GR32_TCRegClassID, 4, 4, 1, 1 },
  { GR32_AD, GR32_ADBits, 306, 2, sizeof(GR32_ADBits), X86_GR32_ADRegClassID, 4, 4, 1, 1 },
  { GR32_NOAX_and_GR32_TC, GR32_NOAX_and_GR32_TCBits, 157, 2, sizeof(GR32_NOAX_and_GR32_TCBits), X86_GR32_NOAX_and_GR32_TCRegClassID, 4, 4, 1, 1 },
  { CCR, CCRBits, 573, 1, sizeof(CCRBits), X86_CCRRegClassID, 4, 4, -1, 0 },
  { GR32_AD_and_GR32_NOAX, GR32_AD_and_GR32_NOAXBits, 630, 1, sizeof(GR32_AD_and_GR32_NOAXBits), X86_GR32_AD_and_GR32_NOAXRegClassID, 4, 4, 1, 1 },
  { RFP64, RFP64Bits, 68, 7, sizeof(RFP64Bits), X86_RFP64RegClassID, 8, 4, 1, 1 },
  { FR64X, FR64XBits, 587, 32, sizeof(FR64XBits), X86_FR64XRegClassID, 8, 8, 1, 1 },
  { GR64, GR64Bits, 79, 17, sizeof(GR64Bits), X86_GR64RegClassID, 8, 8, 1, 1 },
  { CONTROL_REG, CONTROL_REGBits, 354, 16, sizeof(CONTROL_REGBits), X86_CONTROL_REGRegClassID, 8, 8, 1, 1 },
  { FR64, FR64Bits, 74, 16, sizeof(FR64Bits), X86_FR64RegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_8bit, GR64_with_sub_8bitBits, 907, 16, sizeof(GR64_with_sub_8bitBits), X86_GR64_with_sub_8bitRegClassID, 8, 8, 1, 1 },
  { GR64_NOSP, GR64_NOSPBits, 492, 15, sizeof(GR64_NOSPBits), X86_GR64_NOSPRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_NOAX, GR64_with_sub_32bit_in_GR32_NOAXBits, 714, 15, sizeof(GR64_with_sub_32bit_in_GR32_NOAXBits), X86_GR64_with_sub_32bit_in_GR32_NOAXRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSP, GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSPBits, 445, 14, sizeof(GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSPBits), X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOSPRegClassID, 8, 8, 1, 1 },
  { GR64_NOREX, GR64_NOREXBits, 852, 9, sizeof(GR64_NOREXBits), X86_GR64_NOREXRegClassID, 8, 8, 1, 1 },
  { GR64_TC, GR64_TCBits, 224, 9, sizeof(GR64_TCBits), X86_GR64_TCRegClassID, 8, 8, 1, 1 },
  { GR64_NOSP_and_GR64_TC, GR64_NOSP_and_GR64_TCBits, 210, 8, sizeof(GR64_NOSP_and_GR64_TCBits), X86_GR64_NOSP_and_GR64_TCRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_16bit_in_GR16_NOREX, GR64_with_sub_16bit_in_GR16_NOREXBits, 863, 8, sizeof(GR64_with_sub_16bit_in_GR16_NOREXBits), X86_GR64_with_sub_16bit_in_GR16_NOREXRegClassID, 8, 8, 1, 1 },
  { VK64, VK64Bits, 63, 8, sizeof(VK64Bits), X86_VK64RegClassID, 8, 8, 1, 1 },
  { VR64, VR64Bits, 84, 8, sizeof(VR64Bits), X86_VR64RegClassID, 8, 8, 1, 1 },
  { GR64_NOREX_NOSP, GR64_NOREX_NOSPBits, 555, 7, sizeof(GR64_NOREX_NOSPBits), X86_GR64_NOREX_NOSPRegClassID, 8, 8, 1, 1 },
  { GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX, GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAXBits, 747, 7, sizeof(GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAXBits), X86_GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAXRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX, GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits, 804, 7, sizeof(GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits), X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXRegClassID, 8, 8, 1, 1 },
  { VK64WM, VK64WMBits, 419, 7, sizeof(VK64WMBits), X86_VK64WMRegClassID, 8, 8, 1, 1 },
  { GR64_NOREX_and_GR64_TC, GR64_NOREX_and_GR64_TCBits, 260, 6, sizeof(GR64_NOREX_and_GR64_TCBits), X86_GR64_NOREX_and_GR64_TCRegClassID, 8, 8, 1, 1 },
  { GR64_TCW64, GR64_TCW64Bits, 89, 6, sizeof(GR64_TCW64Bits), X86_GR64_TCW64RegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSP, GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSPBits, 502, 6, sizeof(GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSPBits), X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX_NOSPRegClassID, 8, 8, 1, 1 },
  { GR64_NOREX_NOSP_and_GR64_TC, GR64_NOREX_NOSP_and_GR64_TCBits, 232, 5, sizeof(GR64_NOREX_NOSP_and_GR64_TCBits), X86_GR64_NOREX_NOSP_and_GR64_TCRegClassID, 8, 8, 1, 1 },
  { GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAX, GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAXBits, 699, 5, sizeof(GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAXBits), X86_GR64_TCW64_and_GR64_with_sub_32bit_in_GR32_NOAXRegClassID, 8, 8, 1, 1 },
  { GR64_ABCD, GR64_ABCDBits, 324, 4, sizeof(GR64_ABCDBits), X86_GR64_ABCDRegClassID, 8, 8, 1, 1 },
  { GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREX, GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits, 792, 4, sizeof(GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXBits), X86_GR64_TC_and_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_NOREXRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAX, GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAXBits, 652, 3, sizeof(GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAXBits), X86_GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_NOAXRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_TC, GR64_with_sub_32bit_in_GR32_TCBits, 179, 3, sizeof(GR64_with_sub_32bit_in_GR32_TCBits), X86_GR64_with_sub_32bit_in_GR32_TCRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_AD, GR64_with_sub_32bit_in_GR32_ADBits, 283, 2, sizeof(GR64_with_sub_32bit_in_GR32_ADBits), X86_GR64_with_sub_32bit_in_GR32_ADRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TC, GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TCBits, 134, 2, sizeof(GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TCBits), X86_GR64_with_sub_32bit_in_GR32_NOAX_and_GR32_TCRegClassID, 8, 8, 1, 1 },
  { GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAX, GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAXBits, 607, 1, sizeof(GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAXBits), X86_GR64_with_sub_32bit_in_GR32_AD_and_GR32_NOAXRegClassID, 8, 8, 1, 1 },
  { RST, RSTBits, 577, 8, sizeof(RSTBits), X86_RSTRegClassID, 10, 4, 1, 0 },
  { RFP80, RFP80Bits, 0, 7, sizeof(RFP80Bits), X86_RFP80RegClassID, 10, 4, 1, 1 },
  { VR128X, VR128XBits, 600, 32, sizeof(VR128XBits), X86_VR128XRegClassID, 16, 16, 1, 1 },
  { VR128, VR128Bits, 120, 16, sizeof(VR128Bits), X86_VR128RegClassID, 16, 16, 1, 1 },
  { VR256X, VR256XBits, 593, 32, sizeof(VR256XBits), X86_VR256XRegClassID, 32, 32, 1, 1 },
  { VR256, VR256Bits, 114, 16, sizeof(VR256Bits), X86_VR256RegClassID, 32, 32, 1, 1 },
  { VR512, VR512Bits, 10, 32, sizeof(VR512Bits), X86_VR512RegClassID, 64, 64, 1, 1 },
  { VR512_with_sub_xmm_in_FR32, VR512_with_sub_xmm_in_FR32Bits, 27, 16, sizeof(VR512_with_sub_xmm_in_FR32Bits), X86_VR512_with_sub_xmm_in_FR32RegClassID, 64, 64, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC
