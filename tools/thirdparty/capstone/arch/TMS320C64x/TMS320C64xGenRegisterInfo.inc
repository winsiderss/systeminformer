/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|*Target Register Enum Values                                                 *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/


#ifdef GET_REGINFO_ENUM
#undef GET_REGINFO_ENUM

enum {
  TMS320C64x_NoRegister,
  TMS320C64x_AMR = 1,
  TMS320C64x_CSR = 2,
  TMS320C64x_DIER = 3,
  TMS320C64x_DNUM = 4,
  TMS320C64x_ECR = 5,
  TMS320C64x_GFPGFR = 6,
  TMS320C64x_GPLYA = 7,
  TMS320C64x_GPLYB = 8,
  TMS320C64x_ICR = 9,
  TMS320C64x_IER = 10,
  TMS320C64x_IERR = 11,
  TMS320C64x_ILC = 12,
  TMS320C64x_IRP = 13,
  TMS320C64x_ISR = 14,
  TMS320C64x_ISTP = 15,
  TMS320C64x_ITSR = 16,
  TMS320C64x_NRP = 17,
  TMS320C64x_NTSR = 18,
  TMS320C64x_REP = 19,
  TMS320C64x_RILC = 20,
  TMS320C64x_SSR = 21,
  TMS320C64x_TSCH = 22,
  TMS320C64x_TSCL = 23,
  TMS320C64x_TSR = 24,
  TMS320C64x_A0 = 25,
  TMS320C64x_A1 = 26,
  TMS320C64x_A2 = 27,
  TMS320C64x_A3 = 28,
  TMS320C64x_A4 = 29,
  TMS320C64x_A5 = 30,
  TMS320C64x_A6 = 31,
  TMS320C64x_A7 = 32,
  TMS320C64x_A8 = 33,
  TMS320C64x_A9 = 34,
  TMS320C64x_A10 = 35,
  TMS320C64x_A11 = 36,
  TMS320C64x_A12 = 37,
  TMS320C64x_A13 = 38,
  TMS320C64x_A14 = 39,
  TMS320C64x_A15 = 40,
  TMS320C64x_A16 = 41,
  TMS320C64x_A17 = 42,
  TMS320C64x_A18 = 43,
  TMS320C64x_A19 = 44,
  TMS320C64x_A20 = 45,
  TMS320C64x_A21 = 46,
  TMS320C64x_A22 = 47,
  TMS320C64x_A23 = 48,
  TMS320C64x_A24 = 49,
  TMS320C64x_A25 = 50,
  TMS320C64x_A26 = 51,
  TMS320C64x_A27 = 52,
  TMS320C64x_A28 = 53,
  TMS320C64x_A29 = 54,
  TMS320C64x_A30 = 55,
  TMS320C64x_A31 = 56,
  TMS320C64x_B0 = 57,
  TMS320C64x_B1 = 58,
  TMS320C64x_B2 = 59,
  TMS320C64x_B3 = 60,
  TMS320C64x_B4 = 61,
  TMS320C64x_B5 = 62,
  TMS320C64x_B6 = 63,
  TMS320C64x_B7 = 64,
  TMS320C64x_B8 = 65,
  TMS320C64x_B9 = 66,
  TMS320C64x_B10 = 67,
  TMS320C64x_B11 = 68,
  TMS320C64x_B12 = 69,
  TMS320C64x_B13 = 70,
  TMS320C64x_B14 = 71,
  TMS320C64x_B15 = 72,
  TMS320C64x_B16 = 73,
  TMS320C64x_B17 = 74,
  TMS320C64x_B18 = 75,
  TMS320C64x_B19 = 76,
  TMS320C64x_B20 = 77,
  TMS320C64x_B21 = 78,
  TMS320C64x_B22 = 79,
  TMS320C64x_B23 = 80,
  TMS320C64x_B24 = 81,
  TMS320C64x_B25 = 82,
  TMS320C64x_B26 = 83,
  TMS320C64x_B27 = 84,
  TMS320C64x_B28 = 85,
  TMS320C64x_B29 = 86,
  TMS320C64x_B30 = 87,
  TMS320C64x_B31 = 88,
  TMS320C64x_PCE1 = 89,
  TMS320C64x_NUM_TARGET_REGS 	// 90
};

// Register classes
enum {
  TMS320C64x_GPRegsRegClassID = 0,
  TMS320C64x_AFRegsRegClassID = 1,
  TMS320C64x_BFRegsRegClassID = 2,
  TMS320C64x_ControlRegsRegClassID = 3,

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

static MCPhysReg TMS320C64xRegDiffLists[] = {
  /* 0 */ 65535, 0,
};

static uint16_t TMS320C64xSubRegIdxLists[] = {
  /* 0 */ 0,
};

static MCRegisterDesc TMS320C64xRegDesc[] = { // Descriptors
  { 3, 0, 0, 0, 0 },
  { 310, 1, 1, 0, 1 },
  { 319, 1, 1, 0, 1 },
  { 298, 1, 1, 0, 1 },
  { 268, 1, 1, 0, 1 },
  { 290, 1, 1, 0, 1 },
  { 303, 1, 1, 0, 1 },
  { 241, 1, 1, 0, 1 },
  { 247, 1, 1, 0, 1 },
  { 294, 1, 1, 0, 1 },
  { 299, 1, 1, 0, 1 },
  { 314, 1, 1, 0, 1 },
  { 254, 1, 1, 0, 1 },
  { 277, 1, 1, 0, 1 },
  { 323, 1, 1, 0, 1 },
  { 285, 1, 1, 0, 1 },
  { 331, 1, 1, 0, 1 },
  { 281, 1, 1, 0, 1 },
  { 336, 1, 1, 0, 1 },
  { 273, 1, 1, 0, 1 },
  { 253, 1, 1, 0, 1 },
  { 327, 1, 1, 0, 1 },
  { 258, 1, 1, 0, 1 },
  { 263, 1, 1, 0, 1 },
  { 332, 1, 1, 0, 1 },
  { 24, 1, 1, 0, 1 },
  { 54, 1, 1, 0, 1 },
  { 81, 1, 1, 0, 1 },
  { 103, 1, 1, 0, 1 },
  { 125, 1, 1, 0, 1 },
  { 147, 1, 1, 0, 1 },
  { 169, 1, 1, 0, 1 },
  { 191, 1, 1, 0, 1 },
  { 213, 1, 1, 0, 1 },
  { 235, 1, 1, 0, 1 },
  { 0, 1, 1, 0, 1 },
  { 30, 1, 1, 0, 1 },
  { 65, 1, 1, 0, 1 },
  { 87, 1, 1, 0, 1 },
  { 109, 1, 1, 0, 1 },
  { 131, 1, 1, 0, 1 },
  { 153, 1, 1, 0, 1 },
  { 175, 1, 1, 0, 1 },
  { 197, 1, 1, 0, 1 },
  { 219, 1, 1, 0, 1 },
  { 8, 1, 1, 0, 1 },
  { 38, 1, 1, 0, 1 },
  { 73, 1, 1, 0, 1 },
  { 95, 1, 1, 0, 1 },
  { 117, 1, 1, 0, 1 },
  { 139, 1, 1, 0, 1 },
  { 161, 1, 1, 0, 1 },
  { 183, 1, 1, 0, 1 },
  { 205, 1, 1, 0, 1 },
  { 227, 1, 1, 0, 1 },
  { 16, 1, 1, 0, 1 },
  { 46, 1, 1, 0, 1 },
  { 27, 1, 1, 0, 1 },
  { 57, 1, 1, 0, 1 },
  { 84, 1, 1, 0, 1 },
  { 106, 1, 1, 0, 1 },
  { 128, 1, 1, 0, 1 },
  { 150, 1, 1, 0, 1 },
  { 172, 1, 1, 0, 1 },
  { 194, 1, 1, 0, 1 },
  { 216, 1, 1, 0, 1 },
  { 238, 1, 1, 0, 1 },
  { 4, 1, 1, 0, 1 },
  { 34, 1, 1, 0, 1 },
  { 69, 1, 1, 0, 1 },
  { 91, 1, 1, 0, 1 },
  { 113, 1, 1, 0, 1 },
  { 135, 1, 1, 0, 1 },
  { 157, 1, 1, 0, 1 },
  { 179, 1, 1, 0, 1 },
  { 201, 1, 1, 0, 1 },
  { 223, 1, 1, 0, 1 },
  { 12, 1, 1, 0, 1 },
  { 42, 1, 1, 0, 1 },
  { 77, 1, 1, 0, 1 },
  { 99, 1, 1, 0, 1 },
  { 121, 1, 1, 0, 1 },
  { 143, 1, 1, 0, 1 },
  { 165, 1, 1, 0, 1 },
  { 187, 1, 1, 0, 1 },
  { 209, 1, 1, 0, 1 },
  { 231, 1, 1, 0, 1 },
  { 20, 1, 1, 0, 1 },
  { 50, 1, 1, 0, 1 },
  { 60, 1, 1, 0, 1 },
};

// GPRegs Register Class...
static MCPhysReg GPRegs[] = {
  TMS320C64x_A0, TMS320C64x_A1, TMS320C64x_A2, TMS320C64x_A3, TMS320C64x_A4, TMS320C64x_A5, TMS320C64x_A6, TMS320C64x_A7, TMS320C64x_A8, TMS320C64x_A9, TMS320C64x_A10, TMS320C64x_A11, TMS320C64x_A12, TMS320C64x_A13, TMS320C64x_A14, TMS320C64x_A15, TMS320C64x_A16, TMS320C64x_A17, TMS320C64x_A18, TMS320C64x_A19, TMS320C64x_A20, TMS320C64x_A21, TMS320C64x_A22, TMS320C64x_A23, TMS320C64x_A24, TMS320C64x_A25, TMS320C64x_A26, TMS320C64x_A27, TMS320C64x_A28, TMS320C64x_A29, TMS320C64x_A30, TMS320C64x_A31, TMS320C64x_B0, TMS320C64x_B1, TMS320C64x_B2, TMS320C64x_B3, TMS320C64x_B4, TMS320C64x_B5, TMS320C64x_B6, TMS320C64x_B7, TMS320C64x_B8, TMS320C64x_B9, TMS320C64x_B10, TMS320C64x_B11, TMS320C64x_B12, TMS320C64x_B13, TMS320C64x_B14, TMS320C64x_B15, TMS320C64x_B16, TMS320C64x_B17, TMS320C64x_B18, TMS320C64x_B19, TMS320C64x_B20, TMS320C64x_B21, TMS320C64x_B22, TMS320C64x_B23, TMS320C64x_B24, TMS320C64x_B25, TMS320C64x_B26, TMS320C64x_B27, TMS320C64x_B28, TMS320C64x_B29, TMS320C64x_B30, TMS320C64x_B31, 
};

// GPRegs Bit set.
static uint8_t GPRegsBits[] = {
  0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 
};

// AFRegs Register Class...
static MCPhysReg AFRegs[] = {
  TMS320C64x_A0, TMS320C64x_A1, TMS320C64x_A2, TMS320C64x_A3, TMS320C64x_A4, TMS320C64x_A5, TMS320C64x_A6, TMS320C64x_A7, TMS320C64x_A8, TMS320C64x_A9, TMS320C64x_A10, TMS320C64x_A11, TMS320C64x_A12, TMS320C64x_A13, TMS320C64x_A14, TMS320C64x_A15, TMS320C64x_A16, TMS320C64x_A17, TMS320C64x_A18, TMS320C64x_A19, TMS320C64x_A20, TMS320C64x_A21, TMS320C64x_A22, TMS320C64x_A23, TMS320C64x_A24, TMS320C64x_A25, TMS320C64x_A26, TMS320C64x_A27, TMS320C64x_A28, TMS320C64x_A29, TMS320C64x_A30, TMS320C64x_A31, 
};

// AFRegs Bit set.
static uint8_t AFRegsBits[] = {
  0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 
};

// BFRegs Register Class...
static MCPhysReg BFRegs[] = {
  TMS320C64x_B0, TMS320C64x_B1, TMS320C64x_B2, TMS320C64x_B3, TMS320C64x_B4, TMS320C64x_B5, TMS320C64x_B6, TMS320C64x_B7, TMS320C64x_B8, TMS320C64x_B9, TMS320C64x_B10, TMS320C64x_B11, TMS320C64x_B12, TMS320C64x_B13, TMS320C64x_B14, TMS320C64x_B15, TMS320C64x_B16, TMS320C64x_B17, TMS320C64x_B18, TMS320C64x_B19, TMS320C64x_B20, TMS320C64x_B21, TMS320C64x_B22, TMS320C64x_B23, TMS320C64x_B24, TMS320C64x_B25, TMS320C64x_B26, TMS320C64x_B27, TMS320C64x_B28, TMS320C64x_B29, TMS320C64x_B30, TMS320C64x_B31, 
};

// BFRegs Bit set.
static uint8_t BFRegsBits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x01, 
};

// ControlRegs Register Class...
static MCPhysReg ControlRegs[] = {
  TMS320C64x_AMR, TMS320C64x_CSR, TMS320C64x_DIER, TMS320C64x_DNUM, TMS320C64x_ECR, TMS320C64x_GFPGFR, TMS320C64x_GPLYA, TMS320C64x_GPLYB, TMS320C64x_ICR, TMS320C64x_IER, TMS320C64x_IERR, TMS320C64x_ILC, TMS320C64x_IRP, TMS320C64x_ISR, TMS320C64x_ISTP, TMS320C64x_ITSR, TMS320C64x_NRP, TMS320C64x_NTSR, TMS320C64x_PCE1, TMS320C64x_REP, TMS320C64x_RILC, TMS320C64x_SSR, TMS320C64x_TSCH, TMS320C64x_TSCL, TMS320C64x_TSR, 
};

// ControlRegs Bit set.
static uint8_t ControlRegsBits[] = {
  0xfe, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 
};

static MCRegisterClass TMS320C64xMCRegisterClasses[] = {
  { GPRegs, GPRegsBits, 64, sizeof(GPRegsBits), TMS320C64x_GPRegsRegClassID, 4, 4, 1, 1 },
  { AFRegs, AFRegsBits, 32, sizeof(AFRegsBits), TMS320C64x_AFRegsRegClassID, 4, 4, 1, 1 },
  { BFRegs, BFRegsBits, 32, sizeof(BFRegsBits), TMS320C64x_BFRegsRegClassID, 4, 4, 1, 1 },
  { ControlRegs, ControlRegsBits, 25, sizeof(ControlRegsBits), TMS320C64x_ControlRegsRegClassID, 4, 4, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC

