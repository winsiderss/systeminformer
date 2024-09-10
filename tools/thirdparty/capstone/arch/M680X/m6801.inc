
// Additional instructions only supported on M6801/3
static const inst_pageX g_m6801_inst_overlay_table[] = {
	// 0x0x, inherent instructions
	{ 0x04, M680X_INS_LSRD, inh_hid, inh_hid },
	{ 0x05, M680X_INS_ASLD, inh_hid, inh_hid },
	// 0x2x, relative branch instructions
	{ 0x21, M680X_INS_BRN, rel8_hid, inh_hid },
	// 0x3x, inherent instructions
	{ 0x38, M680X_INS_PULX, inh_hid, inh_hid },
	{ 0x3A, M680X_INS_ABX, inh_hid, inh_hid },
	{ 0x3C, M680X_INS_PSHX, inh_hid, inh_hid },
	{ 0x3D, M680X_INS_MUL, inh_hid, inh_hid  },
	// 0x8x, immediate instructions with Register D
	{ 0x83, M680X_INS_SUBD, imm16_hid, inh_hid },
	// 0x9x, direct instructions with register D
	{ 0x93, M680X_INS_SUBD, dir_hid, inh_hid },
	{ 0x9D, M680X_INS_JSR, dir_hid, inh_hid },
	// 0xAx, indexed instructions with Register D
	{ 0xA3, M680X_INS_SUBD, idxX_hid, inh_hid },
	// 0xBx, extended instructions with register D
	{ 0xB3, M680X_INS_SUBD, ext_hid, inh_hid },
	// 0xCx, immediate instructions with register D
	{ 0xC3, M680X_INS_ADDD, imm16_hid, inh_hid },
	{ 0xCC, M680X_INS_LDD, imm16_hid, inh_hid },
	// 0xDx direct instructions with register D
	{ 0xD3, M680X_INS_ADDD, dir_hid, inh_hid },
	{ 0xDC, M680X_INS_LDD, dir_hid, inh_hid },
	{ 0xDD, M680X_INS_STD, dir_hid, inh_hid },
	// 0xEx, indexed instruction with register D
	{ 0xE3, M680X_INS_ADDD, idxX_hid, inh_hid },
	{ 0xEC, M680X_INS_LDD, idxX_hid, inh_hid },
	{ 0xED, M680X_INS_STD, idxX_hid, inh_hid },
	// 0xFx, extended instructions with register D
	{ 0xF3, M680X_INS_ADDD, ext_hid, inh_hid },
	{ 0xFC, M680X_INS_LDD, ext_hid, inh_hid },
	{ 0xFD, M680X_INS_STD, ext_hid, inh_hid },
};

