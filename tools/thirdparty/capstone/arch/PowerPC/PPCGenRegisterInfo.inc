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
  PPC_NoRegister,
  PPC_BP = 1,
  PPC_CARRY = 2,
  PPC_CTR = 3,
  PPC_FP = 4,
  PPC_LR = 5,
  PPC_RM = 6,
  PPC_VRSAVE = 7,
  PPC_ZERO = 8,
  PPC_BP8 = 9,
  PPC_CR0 = 10,
  PPC_CR1 = 11,
  PPC_CR2 = 12,
  PPC_CR3 = 13,
  PPC_CR4 = 14,
  PPC_CR5 = 15,
  PPC_CR6 = 16,
  PPC_CR7 = 17,
  PPC_CTR8 = 18,
  PPC_F0 = 19,
  PPC_F1 = 20,
  PPC_F2 = 21,
  PPC_F3 = 22,
  PPC_F4 = 23,
  PPC_F5 = 24,
  PPC_F6 = 25,
  PPC_F7 = 26,
  PPC_F8 = 27,
  PPC_F9 = 28,
  PPC_F10 = 29,
  PPC_F11 = 30,
  PPC_F12 = 31,
  PPC_F13 = 32,
  PPC_F14 = 33,
  PPC_F15 = 34,
  PPC_F16 = 35,
  PPC_F17 = 36,
  PPC_F18 = 37,
  PPC_F19 = 38,
  PPC_F20 = 39,
  PPC_F21 = 40,
  PPC_F22 = 41,
  PPC_F23 = 42,
  PPC_F24 = 43,
  PPC_F25 = 44,
  PPC_F26 = 45,
  PPC_F27 = 46,
  PPC_F28 = 47,
  PPC_F29 = 48,
  PPC_F30 = 49,
  PPC_F31 = 50,
  PPC_FP8 = 51,
  PPC_LR8 = 52,
  PPC_QF0 = 53,
  PPC_QF1 = 54,
  PPC_QF2 = 55,
  PPC_QF3 = 56,
  PPC_QF4 = 57,
  PPC_QF5 = 58,
  PPC_QF6 = 59,
  PPC_QF7 = 60,
  PPC_QF8 = 61,
  PPC_QF9 = 62,
  PPC_QF10 = 63,
  PPC_QF11 = 64,
  PPC_QF12 = 65,
  PPC_QF13 = 66,
  PPC_QF14 = 67,
  PPC_QF15 = 68,
  PPC_QF16 = 69,
  PPC_QF17 = 70,
  PPC_QF18 = 71,
  PPC_QF19 = 72,
  PPC_QF20 = 73,
  PPC_QF21 = 74,
  PPC_QF22 = 75,
  PPC_QF23 = 76,
  PPC_QF24 = 77,
  PPC_QF25 = 78,
  PPC_QF26 = 79,
  PPC_QF27 = 80,
  PPC_QF28 = 81,
  PPC_QF29 = 82,
  PPC_QF30 = 83,
  PPC_QF31 = 84,
  PPC_R0 = 85,
  PPC_R1 = 86,
  PPC_R2 = 87,
  PPC_R3 = 88,
  PPC_R4 = 89,
  PPC_R5 = 90,
  PPC_R6 = 91,
  PPC_R7 = 92,
  PPC_R8 = 93,
  PPC_R9 = 94,
  PPC_R10 = 95,
  PPC_R11 = 96,
  PPC_R12 = 97,
  PPC_R13 = 98,
  PPC_R14 = 99,
  PPC_R15 = 100,
  PPC_R16 = 101,
  PPC_R17 = 102,
  PPC_R18 = 103,
  PPC_R19 = 104,
  PPC_R20 = 105,
  PPC_R21 = 106,
  PPC_R22 = 107,
  PPC_R23 = 108,
  PPC_R24 = 109,
  PPC_R25 = 110,
  PPC_R26 = 111,
  PPC_R27 = 112,
  PPC_R28 = 113,
  PPC_R29 = 114,
  PPC_R30 = 115,
  PPC_R31 = 116,
  PPC_V0 = 117,
  PPC_V1 = 118,
  PPC_V2 = 119,
  PPC_V3 = 120,
  PPC_V4 = 121,
  PPC_V5 = 122,
  PPC_V6 = 123,
  PPC_V7 = 124,
  PPC_V8 = 125,
  PPC_V9 = 126,
  PPC_V10 = 127,
  PPC_V11 = 128,
  PPC_V12 = 129,
  PPC_V13 = 130,
  PPC_V14 = 131,
  PPC_V15 = 132,
  PPC_V16 = 133,
  PPC_V17 = 134,
  PPC_V18 = 135,
  PPC_V19 = 136,
  PPC_V20 = 137,
  PPC_V21 = 138,
  PPC_V22 = 139,
  PPC_V23 = 140,
  PPC_V24 = 141,
  PPC_V25 = 142,
  PPC_V26 = 143,
  PPC_V27 = 144,
  PPC_V28 = 145,
  PPC_V29 = 146,
  PPC_V30 = 147,
  PPC_V31 = 148,
  PPC_VF0 = 149,
  PPC_VF1 = 150,
  PPC_VF2 = 151,
  PPC_VF3 = 152,
  PPC_VF4 = 153,
  PPC_VF5 = 154,
  PPC_VF6 = 155,
  PPC_VF7 = 156,
  PPC_VF8 = 157,
  PPC_VF9 = 158,
  PPC_VF10 = 159,
  PPC_VF11 = 160,
  PPC_VF12 = 161,
  PPC_VF13 = 162,
  PPC_VF14 = 163,
  PPC_VF15 = 164,
  PPC_VF16 = 165,
  PPC_VF17 = 166,
  PPC_VF18 = 167,
  PPC_VF19 = 168,
  PPC_VF20 = 169,
  PPC_VF21 = 170,
  PPC_VF22 = 171,
  PPC_VF23 = 172,
  PPC_VF24 = 173,
  PPC_VF25 = 174,
  PPC_VF26 = 175,
  PPC_VF27 = 176,
  PPC_VF28 = 177,
  PPC_VF29 = 178,
  PPC_VF30 = 179,
  PPC_VF31 = 180,
  PPC_VSH0 = 181,
  PPC_VSH1 = 182,
  PPC_VSH2 = 183,
  PPC_VSH3 = 184,
  PPC_VSH4 = 185,
  PPC_VSH5 = 186,
  PPC_VSH6 = 187,
  PPC_VSH7 = 188,
  PPC_VSH8 = 189,
  PPC_VSH9 = 190,
  PPC_VSH10 = 191,
  PPC_VSH11 = 192,
  PPC_VSH12 = 193,
  PPC_VSH13 = 194,
  PPC_VSH14 = 195,
  PPC_VSH15 = 196,
  PPC_VSH16 = 197,
  PPC_VSH17 = 198,
  PPC_VSH18 = 199,
  PPC_VSH19 = 200,
  PPC_VSH20 = 201,
  PPC_VSH21 = 202,
  PPC_VSH22 = 203,
  PPC_VSH23 = 204,
  PPC_VSH24 = 205,
  PPC_VSH25 = 206,
  PPC_VSH26 = 207,
  PPC_VSH27 = 208,
  PPC_VSH28 = 209,
  PPC_VSH29 = 210,
  PPC_VSH30 = 211,
  PPC_VSH31 = 212,
  PPC_VSL0 = 213,
  PPC_VSL1 = 214,
  PPC_VSL2 = 215,
  PPC_VSL3 = 216,
  PPC_VSL4 = 217,
  PPC_VSL5 = 218,
  PPC_VSL6 = 219,
  PPC_VSL7 = 220,
  PPC_VSL8 = 221,
  PPC_VSL9 = 222,
  PPC_VSL10 = 223,
  PPC_VSL11 = 224,
  PPC_VSL12 = 225,
  PPC_VSL13 = 226,
  PPC_VSL14 = 227,
  PPC_VSL15 = 228,
  PPC_VSL16 = 229,
  PPC_VSL17 = 230,
  PPC_VSL18 = 231,
  PPC_VSL19 = 232,
  PPC_VSL20 = 233,
  PPC_VSL21 = 234,
  PPC_VSL22 = 235,
  PPC_VSL23 = 236,
  PPC_VSL24 = 237,
  PPC_VSL25 = 238,
  PPC_VSL26 = 239,
  PPC_VSL27 = 240,
  PPC_VSL28 = 241,
  PPC_VSL29 = 242,
  PPC_VSL30 = 243,
  PPC_VSL31 = 244,
  PPC_X0 = 245,
  PPC_X1 = 246,
  PPC_X2 = 247,
  PPC_X3 = 248,
  PPC_X4 = 249,
  PPC_X5 = 250,
  PPC_X6 = 251,
  PPC_X7 = 252,
  PPC_X8 = 253,
  PPC_X9 = 254,
  PPC_X10 = 255,
  PPC_X11 = 256,
  PPC_X12 = 257,
  PPC_X13 = 258,
  PPC_X14 = 259,
  PPC_X15 = 260,
  PPC_X16 = 261,
  PPC_X17 = 262,
  PPC_X18 = 263,
  PPC_X19 = 264,
  PPC_X20 = 265,
  PPC_X21 = 266,
  PPC_X22 = 267,
  PPC_X23 = 268,
  PPC_X24 = 269,
  PPC_X25 = 270,
  PPC_X26 = 271,
  PPC_X27 = 272,
  PPC_X28 = 273,
  PPC_X29 = 274,
  PPC_X30 = 275,
  PPC_X31 = 276,
  PPC_ZERO8 = 277,
  PPC_CR0EQ = 278,
  PPC_CR1EQ = 279,
  PPC_CR2EQ = 280,
  PPC_CR3EQ = 281,
  PPC_CR4EQ = 282,
  PPC_CR5EQ = 283,
  PPC_CR6EQ = 284,
  PPC_CR7EQ = 285,
  PPC_CR0GT = 286,
  PPC_CR1GT = 287,
  PPC_CR2GT = 288,
  PPC_CR3GT = 289,
  PPC_CR4GT = 290,
  PPC_CR5GT = 291,
  PPC_CR6GT = 292,
  PPC_CR7GT = 293,
  PPC_CR0LT = 294,
  PPC_CR1LT = 295,
  PPC_CR2LT = 296,
  PPC_CR3LT = 297,
  PPC_CR4LT = 298,
  PPC_CR5LT = 299,
  PPC_CR6LT = 300,
  PPC_CR7LT = 301,
  PPC_CR0UN = 302,
  PPC_CR1UN = 303,
  PPC_CR2UN = 304,
  PPC_CR3UN = 305,
  PPC_CR4UN = 306,
  PPC_CR5UN = 307,
  PPC_CR6UN = 308,
  PPC_CR7UN = 309,
  PPC_NUM_TARGET_REGS 	// 310
};

// Register classes
enum {
  PPC_GPRCRegClassID = 0,
  PPC_GPRC_NOR0RegClassID = 1,
  PPC_GPRC_and_GPRC_NOR0RegClassID = 2,
  PPC_CRBITRCRegClassID = 3,
  PPC_F4RCRegClassID = 4,
  PPC_CRRCRegClassID = 5,
  PPC_CARRYRCRegClassID = 6,
  PPC_CTRRCRegClassID = 7,
  PPC_VRSAVERCRegClassID = 8,
  PPC_VSFRCRegClassID = 9,
  PPC_G8RCRegClassID = 10,
  PPC_G8RC_NOX0RegClassID = 11,
  PPC_G8RC_and_G8RC_NOX0RegClassID = 12,
  PPC_F8RCRegClassID = 13,
  PPC_VFRCRegClassID = 14,
  PPC_CTRRC8RegClassID = 15,
  PPC_VSRCRegClassID = 16,
  PPC_QSRCRegClassID = 17,
  PPC_VRRCRegClassID = 18,
  PPC_VSHRCRegClassID = 19,
  PPC_VSLRCRegClassID = 20,
  PPC_QBRCRegClassID = 21,
  PPC_QFRCRegClassID = 22,
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

static const MCPhysReg PPCRegDiffLists[] = {
  /* 0 */ 0, 0,
  /* 2 */ 65504, 1, 1, 1, 0,
  /* 7 */ 3, 0,
  /* 9 */ 8, 0,
  /* 11 */ 22, 0,
  /* 13 */ 284, 65528, 65528, 24, 0,
  /* 18 */ 65472, 32, 0,
  /* 21 */ 47, 0,
  /* 23 */ 65504, 64, 0,
  /* 26 */ 73, 0,
  /* 28 */ 34, 160, 0,
  /* 31 */ 269, 0,
  /* 33 */ 64339, 0,
  /* 35 */ 64368, 0,
  /* 37 */ 64401, 0,
  /* 39 */ 64434, 0,
  /* 41 */ 64712, 0,
  /* 43 */ 65244, 0,
  /* 45 */ 65252, 0,
  /* 47 */ 65260, 0,
  /* 49 */ 65267, 0,
  /* 51 */ 65268, 0,
  /* 53 */ 65342, 0,
  /* 55 */ 65364, 0,
  /* 57 */ 65365, 0,
  /* 59 */ 65376, 0,
  /* 61 */ 65461, 0,
  /* 63 */ 65489, 0,
  /* 65 */ 65493, 0,
  /* 67 */ 65502, 0,
  /* 69 */ 65524, 0,
  /* 71 */ 65525, 0,
  /* 73 */ 65528, 0,
  /* 75 */ 65535, 0,
};

static const uint16_t PPCSubRegIdxLists[] = {
  /* 0 */ 1, 0,
  /* 2 */ 3, 2, 0,
  /* 5 */ 6, 5, 4, 7, 0,
};

static MCRegisterDesc PPCRegDesc[] = { // Descriptors
  { 4, 0, 0, 0, 0, 0 },
  { 1109, 1, 9, 1, 1201, 0 },
  { 1266, 1, 1, 1, 1201, 0 },
  { 1166, 1, 1, 1, 1201, 0 },
  { 1112, 1, 21, 1, 1201, 0 },
  { 1163, 1, 1, 1, 1201, 0 },
  { 1053, 1, 1, 1, 1201, 0 },
  { 1046, 1, 1, 1, 1201, 0 },
  { 1104, 1, 31, 1, 1201, 0 },
  { 928, 73, 1, 0, 0, 2 },
  { 120, 13, 1, 5, 36, 6 },
  { 250, 13, 1, 5, 36, 6 },
  { 346, 13, 1, 5, 36, 6 },
  { 442, 13, 1, 5, 36, 6 },
  { 538, 13, 1, 5, 36, 6 },
  { 634, 13, 1, 5, 36, 6 },
  { 730, 13, 1, 5, 36, 6 },
  { 826, 13, 1, 5, 36, 6 },
  { 940, 1, 1, 1, 177, 0 },
  { 103, 1, 28, 1, 177, 0 },
  { 233, 1, 28, 1, 177, 0 },
  { 329, 1, 28, 1, 177, 0 },
  { 425, 1, 28, 1, 177, 0 },
  { 521, 1, 28, 1, 177, 0 },
  { 617, 1, 28, 1, 177, 0 },
  { 713, 1, 28, 1, 177, 0 },
  { 809, 1, 28, 1, 177, 0 },
  { 905, 1, 28, 1, 177, 0 },
  { 1020, 1, 28, 1, 177, 0 },
  { 1, 1, 28, 1, 177, 0 },
  { 131, 1, 28, 1, 177, 0 },
  { 261, 1, 28, 1, 177, 0 },
  { 357, 1, 28, 1, 177, 0 },
  { 453, 1, 28, 1, 177, 0 },
  { 549, 1, 28, 1, 177, 0 },
  { 645, 1, 28, 1, 177, 0 },
  { 741, 1, 28, 1, 177, 0 },
  { 837, 1, 28, 1, 177, 0 },
  { 952, 1, 28, 1, 177, 0 },
  { 35, 1, 28, 1, 177, 0 },
  { 165, 1, 28, 1, 177, 0 },
  { 295, 1, 28, 1, 177, 0 },
  { 391, 1, 28, 1, 177, 0 },
  { 487, 1, 28, 1, 177, 0 },
  { 583, 1, 28, 1, 177, 0 },
  { 679, 1, 28, 1, 177, 0 },
  { 775, 1, 28, 1, 177, 0 },
  { 871, 1, 28, 1, 177, 0 },
  { 986, 1, 28, 1, 177, 0 },
  { 69, 1, 28, 1, 177, 0 },
  { 199, 1, 28, 1, 177, 0 },
  { 932, 63, 1, 0, 112, 2 },
  { 936, 1, 1, 1, 416, 0 },
  { 102, 67, 1, 3, 1105, 4 },
  { 232, 67, 1, 3, 1105, 4 },
  { 328, 67, 1, 3, 1105, 4 },
  { 424, 67, 1, 3, 1105, 4 },
  { 520, 67, 1, 3, 1105, 4 },
  { 616, 67, 1, 3, 1105, 4 },
  { 712, 67, 1, 3, 1105, 4 },
  { 808, 67, 1, 3, 1105, 4 },
  { 904, 67, 1, 3, 1105, 4 },
  { 1019, 67, 1, 3, 1105, 4 },
  { 0, 67, 1, 3, 1105, 4 },
  { 130, 67, 1, 3, 1105, 4 },
  { 260, 67, 1, 3, 1105, 4 },
  { 356, 67, 1, 3, 1105, 4 },
  { 452, 67, 1, 3, 1105, 4 },
  { 548, 67, 1, 3, 1105, 4 },
  { 644, 67, 1, 3, 1105, 4 },
  { 740, 67, 1, 3, 1105, 4 },
  { 836, 67, 1, 3, 1105, 4 },
  { 951, 67, 1, 3, 1105, 4 },
  { 34, 67, 1, 3, 1105, 4 },
  { 164, 67, 1, 3, 1105, 4 },
  { 294, 67, 1, 3, 1105, 4 },
  { 390, 67, 1, 3, 1105, 4 },
  { 486, 67, 1, 3, 1105, 4 },
  { 582, 67, 1, 3, 1105, 4 },
  { 678, 67, 1, 3, 1105, 4 },
  { 774, 67, 1, 3, 1105, 4 },
  { 870, 67, 1, 3, 1105, 4 },
  { 985, 67, 1, 3, 1105, 4 },
  { 68, 67, 1, 3, 1105, 4 },
  { 198, 67, 1, 3, 1105, 4 },
  { 121, 1, 29, 1, 1137, 0 },
  { 251, 1, 29, 1, 1137, 0 },
  { 347, 1, 29, 1, 1137, 0 },
  { 443, 1, 29, 1, 1137, 0 },
  { 539, 1, 29, 1, 1137, 0 },
  { 635, 1, 29, 1, 1137, 0 },
  { 731, 1, 29, 1, 1137, 0 },
  { 827, 1, 29, 1, 1137, 0 },
  { 937, 1, 29, 1, 1137, 0 },
  { 1037, 1, 29, 1, 1137, 0 },
  { 22, 1, 29, 1, 1137, 0 },
  { 152, 1, 29, 1, 1137, 0 },
  { 282, 1, 29, 1, 1137, 0 },
  { 378, 1, 29, 1, 1137, 0 },
  { 474, 1, 29, 1, 1137, 0 },
  { 570, 1, 29, 1, 1137, 0 },
  { 666, 1, 29, 1, 1137, 0 },
  { 762, 1, 29, 1, 1137, 0 },
  { 858, 1, 29, 1, 1137, 0 },
  { 973, 1, 29, 1, 1137, 0 },
  { 56, 1, 29, 1, 1137, 0 },
  { 186, 1, 29, 1, 1137, 0 },
  { 316, 1, 29, 1, 1137, 0 },
  { 412, 1, 29, 1, 1137, 0 },
  { 508, 1, 29, 1, 1137, 0 },
  { 604, 1, 29, 1, 1137, 0 },
  { 700, 1, 29, 1, 1137, 0 },
  { 796, 1, 29, 1, 1137, 0 },
  { 892, 1, 29, 1, 1137, 0 },
  { 1007, 1, 29, 1, 1137, 0 },
  { 90, 1, 29, 1, 1137, 0 },
  { 220, 1, 29, 1, 1137, 0 },
  { 124, 19, 24, 3, 1137, 4 },
  { 254, 19, 24, 3, 1137, 4 },
  { 350, 19, 24, 3, 1137, 4 },
  { 446, 19, 24, 3, 1137, 4 },
  { 542, 19, 24, 3, 1137, 4 },
  { 638, 19, 24, 3, 1137, 4 },
  { 734, 19, 24, 3, 1137, 4 },
  { 830, 19, 24, 3, 1137, 4 },
  { 945, 19, 24, 3, 1137, 4 },
  { 1040, 19, 24, 3, 1137, 4 },
  { 26, 19, 24, 3, 1137, 4 },
  { 156, 19, 24, 3, 1137, 4 },
  { 286, 19, 24, 3, 1137, 4 },
  { 382, 19, 24, 3, 1137, 4 },
  { 478, 19, 24, 3, 1137, 4 },
  { 574, 19, 24, 3, 1137, 4 },
  { 670, 19, 24, 3, 1137, 4 },
  { 766, 19, 24, 3, 1137, 4 },
  { 862, 19, 24, 3, 1137, 4 },
  { 977, 19, 24, 3, 1137, 4 },
  { 60, 19, 24, 3, 1137, 4 },
  { 190, 19, 24, 3, 1137, 4 },
  { 320, 19, 24, 3, 1137, 4 },
  { 416, 19, 24, 3, 1137, 4 },
  { 512, 19, 24, 3, 1137, 4 },
  { 608, 19, 24, 3, 1137, 4 },
  { 704, 19, 24, 3, 1137, 4 },
  { 800, 19, 24, 3, 1137, 4 },
  { 896, 19, 24, 3, 1137, 4 },
  { 1011, 19, 24, 3, 1137, 4 },
  { 94, 19, 24, 3, 1137, 4 },
  { 224, 19, 24, 3, 1137, 4 },
  { 106, 1, 23, 1, 1041, 0 },
  { 236, 1, 23, 1, 1041, 0 },
  { 332, 1, 23, 1, 1041, 0 },
  { 428, 1, 23, 1, 1041, 0 },
  { 524, 1, 23, 1, 1041, 0 },
  { 620, 1, 23, 1, 1041, 0 },
  { 716, 1, 23, 1, 1041, 0 },
  { 812, 1, 23, 1, 1041, 0 },
  { 908, 1, 23, 1, 1041, 0 },
  { 1023, 1, 23, 1, 1041, 0 },
  { 5, 1, 23, 1, 1041, 0 },
  { 135, 1, 23, 1, 1041, 0 },
  { 265, 1, 23, 1, 1041, 0 },
  { 361, 1, 23, 1, 1041, 0 },
  { 457, 1, 23, 1, 1041, 0 },
  { 553, 1, 23, 1, 1041, 0 },
  { 649, 1, 23, 1, 1041, 0 },
  { 745, 1, 23, 1, 1041, 0 },
  { 841, 1, 23, 1, 1041, 0 },
  { 956, 1, 23, 1, 1041, 0 },
  { 39, 1, 23, 1, 1041, 0 },
  { 169, 1, 23, 1, 1041, 0 },
  { 299, 1, 23, 1, 1041, 0 },
  { 395, 1, 23, 1, 1041, 0 },
  { 491, 1, 23, 1, 1041, 0 },
  { 587, 1, 23, 1, 1041, 0 },
  { 683, 1, 23, 1, 1041, 0 },
  { 779, 1, 23, 1, 1041, 0 },
  { 875, 1, 23, 1, 1041, 0 },
  { 990, 1, 23, 1, 1041, 0 },
  { 73, 1, 23, 1, 1041, 0 },
  { 203, 1, 23, 1, 1041, 0 },
  { 110, 18, 1, 2, 977, 4 },
  { 240, 18, 1, 2, 977, 4 },
  { 336, 18, 1, 2, 977, 4 },
  { 432, 18, 1, 2, 977, 4 },
  { 528, 18, 1, 2, 977, 4 },
  { 624, 18, 1, 2, 977, 4 },
  { 720, 18, 1, 2, 977, 4 },
  { 816, 18, 1, 2, 977, 4 },
  { 912, 18, 1, 2, 977, 4 },
  { 1027, 18, 1, 2, 977, 4 },
  { 10, 18, 1, 2, 977, 4 },
  { 140, 18, 1, 2, 977, 4 },
  { 270, 18, 1, 2, 977, 4 },
  { 366, 18, 1, 2, 977, 4 },
  { 462, 18, 1, 2, 977, 4 },
  { 558, 18, 1, 2, 977, 4 },
  { 654, 18, 1, 2, 977, 4 },
  { 750, 18, 1, 2, 977, 4 },
  { 846, 18, 1, 2, 977, 4 },
  { 961, 18, 1, 2, 977, 4 },
  { 44, 18, 1, 2, 977, 4 },
  { 174, 18, 1, 2, 977, 4 },
  { 304, 18, 1, 2, 977, 4 },
  { 400, 18, 1, 2, 977, 4 },
  { 496, 18, 1, 2, 977, 4 },
  { 592, 18, 1, 2, 977, 4 },
  { 688, 18, 1, 2, 977, 4 },
  { 784, 18, 1, 2, 977, 4 },
  { 880, 18, 1, 2, 977, 4 },
  { 995, 18, 1, 2, 977, 4 },
  { 78, 18, 1, 2, 977, 4 },
  { 208, 18, 1, 2, 977, 4 },
  { 115, 53, 1, 3, 881, 4 },
  { 245, 53, 1, 3, 881, 4 },
  { 341, 53, 1, 3, 881, 4 },
  { 437, 53, 1, 3, 881, 4 },
  { 533, 53, 1, 3, 881, 4 },
  { 629, 53, 1, 3, 881, 4 },
  { 725, 53, 1, 3, 881, 4 },
  { 821, 53, 1, 3, 881, 4 },
  { 917, 53, 1, 3, 881, 4 },
  { 1032, 53, 1, 3, 881, 4 },
  { 16, 53, 1, 3, 881, 4 },
  { 146, 53, 1, 3, 881, 4 },
  { 276, 53, 1, 3, 881, 4 },
  { 372, 53, 1, 3, 881, 4 },
  { 468, 53, 1, 3, 881, 4 },
  { 564, 53, 1, 3, 881, 4 },
  { 660, 53, 1, 3, 881, 4 },
  { 756, 53, 1, 3, 881, 4 },
  { 852, 53, 1, 3, 881, 4 },
  { 967, 53, 1, 3, 881, 4 },
  { 50, 53, 1, 3, 881, 4 },
  { 180, 53, 1, 3, 881, 4 },
  { 310, 53, 1, 3, 881, 4 },
  { 406, 53, 1, 3, 881, 4 },
  { 502, 53, 1, 3, 881, 4 },
  { 598, 53, 1, 3, 881, 4 },
  { 694, 53, 1, 3, 881, 4 },
  { 790, 53, 1, 3, 881, 4 },
  { 886, 53, 1, 3, 881, 4 },
  { 1001, 53, 1, 3, 881, 4 },
  { 84, 53, 1, 3, 881, 4 },
  { 214, 53, 1, 3, 881, 4 },
  { 127, 59, 1, 0, 913, 2 },
  { 257, 59, 1, 0, 913, 2 },
  { 353, 59, 1, 0, 913, 2 },
  { 449, 59, 1, 0, 913, 2 },
  { 545, 59, 1, 0, 913, 2 },
  { 641, 59, 1, 0, 913, 2 },
  { 737, 59, 1, 0, 913, 2 },
  { 833, 59, 1, 0, 913, 2 },
  { 948, 59, 1, 0, 913, 2 },
  { 1043, 59, 1, 0, 913, 2 },
  { 30, 59, 1, 0, 913, 2 },
  { 160, 59, 1, 0, 913, 2 },
  { 290, 59, 1, 0, 913, 2 },
  { 386, 59, 1, 0, 913, 2 },
  { 482, 59, 1, 0, 913, 2 },
  { 578, 59, 1, 0, 913, 2 },
  { 674, 59, 1, 0, 913, 2 },
  { 770, 59, 1, 0, 913, 2 },
  { 866, 59, 1, 0, 913, 2 },
  { 981, 59, 1, 0, 913, 2 },
  { 64, 59, 1, 0, 913, 2 },
  { 194, 59, 1, 0, 913, 2 },
  { 324, 59, 1, 0, 913, 2 },
  { 420, 59, 1, 0, 913, 2 },
  { 516, 59, 1, 0, 913, 2 },
  { 612, 59, 1, 0, 913, 2 },
  { 708, 59, 1, 0, 913, 2 },
  { 804, 59, 1, 0, 913, 2 },
  { 900, 59, 1, 0, 913, 2 },
  { 1015, 59, 1, 0, 913, 2 },
  { 98, 59, 1, 0, 913, 2 },
  { 228, 59, 1, 0, 913, 2 },
  { 922, 49, 1, 0, 659, 2 },
  { 1115, 1, 51, 1, 659, 0 },
  { 1121, 1, 51, 1, 628, 0 },
  { 1127, 1, 51, 1, 628, 0 },
  { 1133, 1, 51, 1, 628, 0 },
  { 1139, 1, 51, 1, 628, 0 },
  { 1145, 1, 51, 1, 628, 0 },
  { 1151, 1, 51, 1, 628, 0 },
  { 1157, 1, 51, 1, 628, 0 },
  { 1170, 1, 47, 1, 596, 0 },
  { 1176, 1, 47, 1, 596, 0 },
  { 1182, 1, 47, 1, 596, 0 },
  { 1188, 1, 47, 1, 596, 0 },
  { 1194, 1, 47, 1, 596, 0 },
  { 1200, 1, 47, 1, 596, 0 },
  { 1206, 1, 47, 1, 596, 0 },
  { 1212, 1, 47, 1, 596, 0 },
  { 1218, 1, 45, 1, 564, 0 },
  { 1224, 1, 45, 1, 564, 0 },
  { 1230, 1, 45, 1, 564, 0 },
  { 1236, 1, 45, 1, 564, 0 },
  { 1242, 1, 45, 1, 564, 0 },
  { 1248, 1, 45, 1, 564, 0 },
  { 1254, 1, 45, 1, 564, 0 },
  { 1260, 1, 45, 1, 564, 0 },
  { 1056, 1, 43, 1, 532, 0 },
  { 1062, 1, 43, 1, 532, 0 },
  { 1068, 1, 43, 1, 532, 0 },
  { 1074, 1, 43, 1, 532, 0 },
  { 1080, 1, 43, 1, 532, 0 },
  { 1086, 1, 43, 1, 532, 0 },
  { 1092, 1, 43, 1, 532, 0 },
  { 1098, 1, 43, 1, 532, 0 },
};


  // GPRC Register Class...
  static const MCPhysReg GPRC[] = {
    PPC_R2, PPC_R3, PPC_R4, PPC_R5, PPC_R6, PPC_R7, PPC_R8, PPC_R9, PPC_R10, PPC_R11, PPC_R12, PPC_R30, PPC_R29, PPC_R28, PPC_R27, PPC_R26, PPC_R25, PPC_R24, PPC_R23, PPC_R22, PPC_R21, PPC_R20, PPC_R19, PPC_R18, PPC_R17, PPC_R16, PPC_R15, PPC_R14, PPC_R13, PPC_R31, PPC_R0, PPC_R1, PPC_FP, PPC_BP, 
  };

  // GPRC Bit set.
  static uint8_t GPRCBits[] = {
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // GPRC_NOR0 Register Class...
  static const MCPhysReg GPRC_NOR0[] = {
    PPC_R2, PPC_R3, PPC_R4, PPC_R5, PPC_R6, PPC_R7, PPC_R8, PPC_R9, PPC_R10, PPC_R11, PPC_R12, PPC_R30, PPC_R29, PPC_R28, PPC_R27, PPC_R26, PPC_R25, PPC_R24, PPC_R23, PPC_R22, PPC_R21, PPC_R20, PPC_R19, PPC_R18, PPC_R17, PPC_R16, PPC_R15, PPC_R14, PPC_R13, PPC_R31, PPC_R1, PPC_FP, PPC_BP, PPC_ZERO, 
  };

  // GPRC_NOR0 Bit set.
  static uint8_t GPRC_NOR0Bits[] = {
    0x12, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // GPRC_and_GPRC_NOR0 Register Class...
  static const MCPhysReg GPRC_and_GPRC_NOR0[] = {
    PPC_R2, PPC_R3, PPC_R4, PPC_R5, PPC_R6, PPC_R7, PPC_R8, PPC_R9, PPC_R10, PPC_R11, PPC_R12, PPC_R30, PPC_R29, PPC_R28, PPC_R27, PPC_R26, PPC_R25, PPC_R24, PPC_R23, PPC_R22, PPC_R21, PPC_R20, PPC_R19, PPC_R18, PPC_R17, PPC_R16, PPC_R15, PPC_R14, PPC_R13, PPC_R31, PPC_R1, PPC_FP, PPC_BP, 
  };

  // GPRC_and_GPRC_NOR0 Bit set.
  static uint8_t GPRC_and_GPRC_NOR0Bits[] = {
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // CRBITRC Register Class...
  static const MCPhysReg CRBITRC[] = {
    PPC_CR2LT, PPC_CR2GT, PPC_CR2EQ, PPC_CR2UN, PPC_CR3LT, PPC_CR3GT, PPC_CR3EQ, PPC_CR3UN, PPC_CR4LT, PPC_CR4GT, PPC_CR4EQ, PPC_CR4UN, PPC_CR5LT, PPC_CR5GT, PPC_CR5EQ, PPC_CR5UN, PPC_CR6LT, PPC_CR6GT, PPC_CR6EQ, PPC_CR6UN, PPC_CR7LT, PPC_CR7GT, PPC_CR7EQ, PPC_CR7UN, PPC_CR1LT, PPC_CR1GT, PPC_CR1EQ, PPC_CR1UN, PPC_CR0LT, PPC_CR0GT, PPC_CR0EQ, PPC_CR0UN, 
  };

  // CRBITRC Bit set.
  static uint8_t CRBITRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x3f, 
  };

  // F4RC Register Class...
  static const MCPhysReg F4RC[] = {
    PPC_F0, PPC_F1, PPC_F2, PPC_F3, PPC_F4, PPC_F5, PPC_F6, PPC_F7, PPC_F8, PPC_F9, PPC_F10, PPC_F11, PPC_F12, PPC_F13, PPC_F31, PPC_F30, PPC_F29, PPC_F28, PPC_F27, PPC_F26, PPC_F25, PPC_F24, PPC_F23, PPC_F22, PPC_F21, PPC_F20, PPC_F19, PPC_F18, PPC_F17, PPC_F16, PPC_F15, PPC_F14, 
  };

  // F4RC Bit set.
  static uint8_t F4RCBits[] = {
    0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x07, 
  };

  // CRRC Register Class...
  static const MCPhysReg CRRC[] = {
    PPC_CR0, PPC_CR1, PPC_CR5, PPC_CR6, PPC_CR7, PPC_CR2, PPC_CR3, PPC_CR4, 
  };

  // CRRC Bit set.
  static uint8_t CRRCBits[] = {
    0x00, 0xfc, 0x03, 
  };

  // CARRYRC Register Class...
  static const MCPhysReg CARRYRC[] = {
    PPC_CARRY, 
  };

  // CARRYRC Bit set.
  static const uint8_t CARRYRCBits[] = {
    0x04, 
  };

  // CTRRC Register Class...
  static const MCPhysReg CTRRC[] = {
    PPC_CTR, 
  };

  // CTRRC Bit set.
  static uint8_t CTRRCBits[] = {
    0x08, 
  };

  // VRSAVERC Register Class...
  static const MCPhysReg VRSAVERC[] = {
    PPC_VRSAVE, 
  };

  // VRSAVERC Bit set.
  static uint8_t VRSAVERCBits[] = {
    0x80, 
  };

  // VSFRC Register Class...
  static const MCPhysReg VSFRC[] = {
    PPC_F0, PPC_F1, PPC_F2, PPC_F3, PPC_F4, PPC_F5, PPC_F6, PPC_F7, PPC_F8, PPC_F9, PPC_F10, PPC_F11, PPC_F12, PPC_F13, PPC_F31, PPC_F30, PPC_F29, PPC_F28, PPC_F27, PPC_F26, PPC_F25, PPC_F24, PPC_F23, PPC_F22, PPC_F21, PPC_F20, PPC_F19, PPC_F18, PPC_F17, PPC_F16, PPC_F15, PPC_F14, PPC_VF2, PPC_VF3, PPC_VF4, PPC_VF5, PPC_VF0, PPC_VF1, PPC_VF6, PPC_VF7, PPC_VF8, PPC_VF9, PPC_VF10, PPC_VF11, PPC_VF12, PPC_VF13, PPC_VF14, PPC_VF15, PPC_VF16, PPC_VF17, PPC_VF18, PPC_VF19, PPC_VF31, PPC_VF30, PPC_VF29, PPC_VF28, PPC_VF27, PPC_VF26, PPC_VF25, PPC_VF24, PPC_VF23, PPC_VF22, PPC_VF21, PPC_VF20, 
  };

  // VSFRC Bit set.
  static uint8_t VSFRCBits[] = {
    0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // G8RC Register Class...
  static const MCPhysReg G8RC[] = {
    PPC_X2, PPC_X3, PPC_X4, PPC_X5, PPC_X6, PPC_X7, PPC_X8, PPC_X9, PPC_X10, PPC_X11, PPC_X12, PPC_X30, PPC_X29, PPC_X28, PPC_X27, PPC_X26, PPC_X25, PPC_X24, PPC_X23, PPC_X22, PPC_X21, PPC_X20, PPC_X19, PPC_X18, PPC_X17, PPC_X16, PPC_X15, PPC_X14, PPC_X31, PPC_X13, PPC_X0, PPC_X1, PPC_FP8, PPC_BP8, 
  };

  // G8RC Bit set.
  static uint8_t G8RCBits[] = {
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // G8RC_NOX0 Register Class...
  static const MCPhysReg G8RC_NOX0[] = {
    PPC_X2, PPC_X3, PPC_X4, PPC_X5, PPC_X6, PPC_X7, PPC_X8, PPC_X9, PPC_X10, PPC_X11, PPC_X12, PPC_X30, PPC_X29, PPC_X28, PPC_X27, PPC_X26, PPC_X25, PPC_X24, PPC_X23, PPC_X22, PPC_X21, PPC_X20, PPC_X19, PPC_X18, PPC_X17, PPC_X16, PPC_X15, PPC_X14, PPC_X31, PPC_X13, PPC_X1, PPC_FP8, PPC_BP8, PPC_ZERO8, 
  };

  // G8RC_NOX0 Bit set.
  static uint8_t G8RC_NOX0Bits[] = {
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x3f, 
  };

  // G8RC_and_G8RC_NOX0 Register Class...
  static const MCPhysReg G8RC_and_G8RC_NOX0[] = {
    PPC_X2, PPC_X3, PPC_X4, PPC_X5, PPC_X6, PPC_X7, PPC_X8, PPC_X9, PPC_X10, PPC_X11, PPC_X12, PPC_X30, PPC_X29, PPC_X28, PPC_X27, PPC_X26, PPC_X25, PPC_X24, PPC_X23, PPC_X22, PPC_X21, PPC_X20, PPC_X19, PPC_X18, PPC_X17, PPC_X16, PPC_X15, PPC_X14, PPC_X31, PPC_X13, PPC_X1, PPC_FP8, PPC_BP8, 
  };

  // G8RC_and_G8RC_NOX0 Bit set.
  static uint8_t G8RC_and_G8RC_NOX0Bits[] = {
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // F8RC Register Class...
  static const MCPhysReg F8RC[] = {
    PPC_F0, PPC_F1, PPC_F2, PPC_F3, PPC_F4, PPC_F5, PPC_F6, PPC_F7, PPC_F8, PPC_F9, PPC_F10, PPC_F11, PPC_F12, PPC_F13, PPC_F31, PPC_F30, PPC_F29, PPC_F28, PPC_F27, PPC_F26, PPC_F25, PPC_F24, PPC_F23, PPC_F22, PPC_F21, PPC_F20, PPC_F19, PPC_F18, PPC_F17, PPC_F16, PPC_F15, PPC_F14, 
  };

  // F8RC Bit set.
  static uint8_t F8RCBits[] = {
    0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x07, 
  };

  // VFRC Register Class...
  static const MCPhysReg VFRC[] = {
    PPC_VF2, PPC_VF3, PPC_VF4, PPC_VF5, PPC_VF0, PPC_VF1, PPC_VF6, PPC_VF7, PPC_VF8, PPC_VF9, PPC_VF10, PPC_VF11, PPC_VF12, PPC_VF13, PPC_VF14, PPC_VF15, PPC_VF16, PPC_VF17, PPC_VF18, PPC_VF19, PPC_VF31, PPC_VF30, PPC_VF29, PPC_VF28, PPC_VF27, PPC_VF26, PPC_VF25, PPC_VF24, PPC_VF23, PPC_VF22, PPC_VF21, PPC_VF20, 
  };

  // VFRC Bit set.
  static uint8_t VFRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // CTRRC8 Register Class...
  static const MCPhysReg CTRRC8[] = {
    PPC_CTR8, 
  };

  // CTRRC8 Bit set.
  static uint8_t CTRRC8Bits[] = {
    0x00, 0x00, 0x04, 
  };

  // VSRC Register Class...
  static const MCPhysReg VSRC[] = {
    PPC_VSL0, PPC_VSL1, PPC_VSL2, PPC_VSL3, PPC_VSL4, PPC_VSL5, PPC_VSL6, PPC_VSL7, PPC_VSL8, PPC_VSL9, PPC_VSL10, PPC_VSL11, PPC_VSL12, PPC_VSL13, PPC_VSL31, PPC_VSL30, PPC_VSL29, PPC_VSL28, PPC_VSL27, PPC_VSL26, PPC_VSL25, PPC_VSL24, PPC_VSL23, PPC_VSL22, PPC_VSL21, PPC_VSL20, PPC_VSL19, PPC_VSL18, PPC_VSL17, PPC_VSL16, PPC_VSL15, PPC_VSL14, PPC_VSH2, PPC_VSH3, PPC_VSH4, PPC_VSH5, PPC_VSH0, PPC_VSH1, PPC_VSH6, PPC_VSH7, PPC_VSH8, PPC_VSH9, PPC_VSH10, PPC_VSH11, PPC_VSH12, PPC_VSH13, PPC_VSH14, PPC_VSH15, PPC_VSH16, PPC_VSH17, PPC_VSH18, PPC_VSH19, PPC_VSH31, PPC_VSH30, PPC_VSH29, PPC_VSH28, PPC_VSH27, PPC_VSH26, PPC_VSH25, PPC_VSH24, PPC_VSH23, PPC_VSH22, PPC_VSH21, PPC_VSH20, 
  };

  // VSRC Bit set.
  static uint8_t VSRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 
  };

  // QSRC Register Class...
  static MCPhysReg QSRC[] = {
    PPC_QF0, PPC_QF1, PPC_QF2, PPC_QF3, PPC_QF4, PPC_QF5, PPC_QF6, PPC_QF7, PPC_QF8, PPC_QF9, PPC_QF10, PPC_QF11, PPC_QF12, PPC_QF13, PPC_QF31, PPC_QF30, PPC_QF29, PPC_QF28, PPC_QF27, PPC_QF26, PPC_QF25, PPC_QF24, PPC_QF23, PPC_QF22, PPC_QF21, PPC_QF20, PPC_QF19, PPC_QF18, PPC_QF17, PPC_QF16, PPC_QF15, PPC_QF14, 
  };

  // QSRC Bit set.
  static uint8_t QSRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // VRRC Register Class...
  static const MCPhysReg VRRC[] = {
    PPC_V2, PPC_V3, PPC_V4, PPC_V5, PPC_V0, PPC_V1, PPC_V6, PPC_V7, PPC_V8, PPC_V9, PPC_V10, PPC_V11, PPC_V12, PPC_V13, PPC_V14, PPC_V15, PPC_V16, PPC_V17, PPC_V18, PPC_V19, PPC_V31, PPC_V30, PPC_V29, PPC_V28, PPC_V27, PPC_V26, PPC_V25, PPC_V24, PPC_V23, PPC_V22, PPC_V21, PPC_V20, 
  };

  // VRRC Bit set.
  static uint8_t VRRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // VSHRC Register Class...
  static const MCPhysReg VSHRC[] = {
    PPC_VSH2, PPC_VSH3, PPC_VSH4, PPC_VSH5, PPC_VSH0, PPC_VSH1, PPC_VSH6, PPC_VSH7, PPC_VSH8, PPC_VSH9, PPC_VSH10, PPC_VSH11, PPC_VSH12, PPC_VSH13, PPC_VSH14, PPC_VSH15, PPC_VSH16, PPC_VSH17, PPC_VSH18, PPC_VSH19, PPC_VSH31, PPC_VSH30, PPC_VSH29, PPC_VSH28, PPC_VSH27, PPC_VSH26, PPC_VSH25, PPC_VSH24, PPC_VSH23, PPC_VSH22, PPC_VSH21, PPC_VSH20, 
  };

  // VSHRC Bit set.
  static uint8_t VSHRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // VSLRC Register Class...
  static const MCPhysReg VSLRC[] = {
    PPC_VSL0, PPC_VSL1, PPC_VSL2, PPC_VSL3, PPC_VSL4, PPC_VSL5, PPC_VSL6, PPC_VSL7, PPC_VSL8, PPC_VSL9, PPC_VSL10, PPC_VSL11, PPC_VSL12, PPC_VSL13, PPC_VSL31, PPC_VSL30, PPC_VSL29, PPC_VSL28, PPC_VSL27, PPC_VSL26, PPC_VSL25, PPC_VSL24, PPC_VSL23, PPC_VSL22, PPC_VSL21, PPC_VSL20, PPC_VSL19, PPC_VSL18, PPC_VSL17, PPC_VSL16, PPC_VSL15, PPC_VSL14, 
  };

  // VSLRC Bit set.
  static uint8_t VSLRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // QBRC Register Class...
  static MCPhysReg QBRC[] = {
    PPC_QF0, PPC_QF1, PPC_QF2, PPC_QF3, PPC_QF4, PPC_QF5, PPC_QF6, PPC_QF7, PPC_QF8, PPC_QF9, PPC_QF10, PPC_QF11, PPC_QF12, PPC_QF13, PPC_QF31, PPC_QF30, PPC_QF29, PPC_QF28, PPC_QF27, PPC_QF26, PPC_QF25, PPC_QF24, PPC_QF23, PPC_QF22, PPC_QF21, PPC_QF20, PPC_QF19, PPC_QF18, PPC_QF17, PPC_QF16, PPC_QF15, PPC_QF14, 
  };

  // QBRC Bit set.
  static uint8_t QBRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };

  // QFRC Register Class...
  static MCPhysReg QFRC[] = {
    PPC_QF0, PPC_QF1, PPC_QF2, PPC_QF3, PPC_QF4, PPC_QF5, PPC_QF6, PPC_QF7, PPC_QF8, PPC_QF9, PPC_QF10, PPC_QF11, PPC_QF12, PPC_QF13, PPC_QF31, PPC_QF30, PPC_QF29, PPC_QF28, PPC_QF27, PPC_QF26, PPC_QF25, PPC_QF24, PPC_QF23, PPC_QF22, PPC_QF21, PPC_QF20, PPC_QF19, PPC_QF18, PPC_QF17, PPC_QF16, PPC_QF15, PPC_QF14, 
  };

  // QFRC Bit set.
  static uint8_t QFRCBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f, 
  };


static MCRegisterClass PPCMCRegisterClasses[] = {
  { GPRC, GPRCBits, 102, 34, sizeof(GPRCBits), PPC_GPRCRegClassID, 4, 4, 1, 1 },
  { GPRC_NOR0, GPRC_NOR0Bits, 9, 34, sizeof(GPRC_NOR0Bits), PPC_GPRC_NOR0RegClassID, 4, 4, 1, 1 },
  { GPRC_and_GPRC_NOR0, GPRC_and_GPRC_NOR0Bits, 0, 33, sizeof(GPRC_and_GPRC_NOR0Bits), PPC_GPRC_and_GPRC_NOR0RegClassID, 4, 4, 1, 1 },
  { CRBITRC, CRBITRCBits, 133, 32, sizeof(CRBITRCBits), PPC_CRBITRCRegClassID, 4, 4, 1, 1 },
  { F4RC, F4RCBits, 45, 32, sizeof(F4RCBits), PPC_F4RCRegClassID, 4, 4, 1, 1 },
  { CRRC, CRRCBits, 107, 8, sizeof(CRRCBits), PPC_CRRCRegClassID, 4, 4, 1, 1 },
  { CARRYRC, CARRYRCBits, 141, 1, sizeof(CARRYRCBits), PPC_CARRYRCRegClassID, 4, 4, -1, 1 },
  { CTRRC, CTRRCBits, 112, 1, sizeof(CTRRCBits), PPC_CTRRCRegClassID, 4, 4, 1, 0 },
  { VRSAVERC, VRSAVERCBits, 65, 1, sizeof(VRSAVERCBits), PPC_VRSAVERCRegClassID, 4, 4, 1, 1 },
  { VSFRC, VSFRCBits, 79, 64, sizeof(VSFRCBits), PPC_VSFRCRegClassID, 8, 8, 1, 1 },
  { G8RC, G8RCBits, 55, 34, sizeof(G8RCBits), PPC_G8RCRegClassID, 8, 8, 1, 1 },
  { G8RC_NOX0, G8RC_NOX0Bits, 28, 34, sizeof(G8RC_NOX0Bits), PPC_G8RC_NOX0RegClassID, 8, 8, 1, 1 },
  { G8RC_and_G8RC_NOX0, G8RC_and_G8RC_NOX0Bits, 19, 33, sizeof(G8RC_and_G8RC_NOX0Bits), PPC_G8RC_and_G8RC_NOX0RegClassID, 8, 8, 1, 1 },
  { F8RC, F8RCBits, 50, 32, sizeof(F8RCBits), PPC_F8RCRegClassID, 8, 8, 1, 1 },
  { VFRC, VFRCBits, 85, 32, sizeof(VFRCBits), PPC_VFRCRegClassID, 8, 8, 1, 1 },
  { CTRRC8, CTRRC8Bits, 38, 1, sizeof(CTRRC8Bits), PPC_CTRRC8RegClassID, 8, 8, 1, 0 },
  { VSRC, VSRCBits, 128, 64, sizeof(VSRCBits), PPC_VSRCRegClassID, 16, 16, 1, 1 },
  { QSRC, QSRCBits, 123, 32, sizeof(QSRCBits), PPC_QSRCRegClassID, 16, 16, 1, 1 },
  { VRRC, VRRCBits, 118, 32, sizeof(VRRCBits), PPC_VRRCRegClassID, 16, 16, 1, 1 },
  { VSHRC, VSHRCBits, 90, 32, sizeof(VSHRCBits), PPC_VSHRCRegClassID, 16, 16, 1, 1 },
  { VSLRC, VSLRCBits, 96, 32, sizeof(VSLRCBits), PPC_VSLRCRegClassID, 16, 16, 1, 1 },
  { QBRC, QBRCBits, 60, 32, sizeof(QBRCBits), PPC_QBRCRegClassID, 32, 32, 1, 1 },
  { QFRC, QFRCBits, 74, 32, sizeof(QFRCBits), PPC_QFRCRegClassID, 32, 32, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC
