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
  XCore_NoRegister,
  XCore_CP = 1,
  XCore_DP = 2,
  XCore_LR = 3,
  XCore_SP = 4,
  XCore_R0 = 5,
  XCore_R1 = 6,
  XCore_R2 = 7,
  XCore_R3 = 8,
  XCore_R4 = 9,
  XCore_R5 = 10,
  XCore_R6 = 11,
  XCore_R7 = 12,
  XCore_R8 = 13,
  XCore_R9 = 14,
  XCore_R10 = 15,
  XCore_R11 = 16,
  XCore_NUM_TARGET_REGS 	// 17
};

// Register classes
enum {
  XCore_RRegsRegClassID = 0,
  XCore_GRRegsRegClassID = 1
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

static const MCPhysReg XCoreRegDiffLists[] = {
  /* 0 */ 65535, 0,
};

static const uint16_t XCoreSubRegIdxLists[] = {
  /* 0 */ 0,
};

static MCRegisterDesc XCoreRegDesc[] = { // Descriptors
  { 3, 0, 0, 0, 0, 0 },
  { 38, 1, 1, 0, 1, 0 },
  { 41, 1, 1, 0, 1, 0 },
  { 47, 1, 1, 0, 1, 0 },
  { 44, 1, 1, 0, 1, 0 },
  { 4, 1, 1, 0, 1, 0 },
  { 11, 1, 1, 0, 1, 0 },
  { 14, 1, 1, 0, 1, 0 },
  { 17, 1, 1, 0, 1, 0 },
  { 20, 1, 1, 0, 1, 0 },
  { 23, 1, 1, 0, 1, 0 },
  { 26, 1, 1, 0, 1, 0 },
  { 29, 1, 1, 0, 1, 0 },
  { 32, 1, 1, 0, 1, 0 },
  { 35, 1, 1, 0, 1, 0 },
  { 0, 1, 1, 0, 1, 0 },
  { 7, 1, 1, 0, 1, 0 },
};

  // RRegs Register Class...
  static const MCPhysReg RRegs[] = {
    XCore_R0, XCore_R1, XCore_R2, XCore_R3, XCore_R4, XCore_R5, XCore_R6, XCore_R7, XCore_R8, XCore_R9, XCore_R10, XCore_R11, XCore_CP, XCore_DP, XCore_SP, XCore_LR, 
  };

  // RRegs Bit set.
  static const uint8_t RRegsBits[] = {
    0xfe, 0xff, 0x01, 
  };

  // GRRegs Register Class...
  static const MCPhysReg GRRegs[] = {
    XCore_R0, XCore_R1, XCore_R2, XCore_R3, XCore_R4, XCore_R5, XCore_R6, XCore_R7, XCore_R8, XCore_R9, XCore_R10, XCore_R11, 
  };

  // GRRegs Bit set.
  static const uint8_t GRRegsBits[] = {
    0xe0, 0xff, 0x01, 
  };

static MCRegisterClass XCoreMCRegisterClasses[] = {
  { RRegs, RRegsBits, 1, 16, sizeof(RRegsBits), XCore_RRegsRegClassID, 4, 4, 1, 0 },
  { GRRegs, GRRegsBits, 0, 12, sizeof(GRRegsBits), XCore_GRRegsRegClassID, 4, 4, 1, 1 },
};

#endif // GET_REGINFO_MC_DESC
