
// Additional instructions only supported on HD6301/3
static const inst_pageX g_hd6301_inst_overlay_table[] = {
	{ 0x18, M680X_INS_XGDX, inh_hid, inh_hid },
	{ 0x1a, M680X_INS_SLP, inh_hid, inh_hid },
	{ 0x61, M680X_INS_AIM, imm8_hid, idxX_hid },
	{ 0x62, M680X_INS_OIM, imm8_hid, idxX_hid },
	{ 0x65, M680X_INS_EIM, imm8_hid, idxX_hid },
	{ 0x6B, M680X_INS_TIM, imm8_hid, idxX_hid },
	{ 0x71, M680X_INS_AIM, imm8_hid, dir_hid },
	{ 0x72, M680X_INS_OIM, imm8_hid, dir_hid },
	{ 0x75, M680X_INS_EIM, imm8_hid, dir_hid },
	{ 0x7B, M680X_INS_TIM, imm8_hid, dir_hid },
};

