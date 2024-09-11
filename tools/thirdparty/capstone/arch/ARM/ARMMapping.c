/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_ARM

#include <stdio.h>	// debug
#include <string.h>

#include "../../cs_priv.h"

#include "ARMMapping.h"

#define GET_INSTRINFO_ENUM
#include "ARMGenInstrInfo.inc"

#ifndef CAPSTONE_DIET
static const name_map reg_name_maps[] = {
	{ ARM_REG_INVALID, NULL },
	{ ARM_REG_APSR, "apsr"},
	{ ARM_REG_APSR_NZCV, "apsr_nzcv"},
	{ ARM_REG_CPSR, "cpsr"},
	{ ARM_REG_FPEXC, "fpexc"},
	{ ARM_REG_FPINST, "fpinst"},
	{ ARM_REG_FPSCR, "fpscr"},
	{ ARM_REG_FPSCR_NZCV, "fpscr_nzcv"},
	{ ARM_REG_FPSID, "fpsid"},
	{ ARM_REG_ITSTATE, "itstate"},
	{ ARM_REG_LR, "lr"},
	{ ARM_REG_PC, "pc"},
	{ ARM_REG_SP, "sp"},
	{ ARM_REG_SPSR, "spsr"},
	{ ARM_REG_D0, "d0"},
	{ ARM_REG_D1, "d1"},
	{ ARM_REG_D2, "d2"},
	{ ARM_REG_D3, "d3"},
	{ ARM_REG_D4, "d4"},
	{ ARM_REG_D5, "d5"},
	{ ARM_REG_D6, "d6"},
	{ ARM_REG_D7, "d7"},
	{ ARM_REG_D8, "d8"},
	{ ARM_REG_D9, "d9"},
	{ ARM_REG_D10, "d10"},
	{ ARM_REG_D11, "d11"},
	{ ARM_REG_D12, "d12"},
	{ ARM_REG_D13, "d13"},
	{ ARM_REG_D14, "d14"},
	{ ARM_REG_D15, "d15"},
	{ ARM_REG_D16, "d16"},
	{ ARM_REG_D17, "d17"},
	{ ARM_REG_D18, "d18"},
	{ ARM_REG_D19, "d19"},
	{ ARM_REG_D20, "d20"},
	{ ARM_REG_D21, "d21"},
	{ ARM_REG_D22, "d22"},
	{ ARM_REG_D23, "d23"},
	{ ARM_REG_D24, "d24"},
	{ ARM_REG_D25, "d25"},
	{ ARM_REG_D26, "d26"},
	{ ARM_REG_D27, "d27"},
	{ ARM_REG_D28, "d28"},
	{ ARM_REG_D29, "d29"},
	{ ARM_REG_D30, "d30"},
	{ ARM_REG_D31, "d31"},
	{ ARM_REG_FPINST2, "fpinst2"},
	{ ARM_REG_MVFR0, "mvfr0"},
	{ ARM_REG_MVFR1, "mvfr1"},
	{ ARM_REG_MVFR2, "mvfr2"},
	{ ARM_REG_Q0, "q0"},
	{ ARM_REG_Q1, "q1"},
	{ ARM_REG_Q2, "q2"},
	{ ARM_REG_Q3, "q3"},
	{ ARM_REG_Q4, "q4"},
	{ ARM_REG_Q5, "q5"},
	{ ARM_REG_Q6, "q6"},
	{ ARM_REG_Q7, "q7"},
	{ ARM_REG_Q8, "q8"},
	{ ARM_REG_Q9, "q9"},
	{ ARM_REG_Q10, "q10"},
	{ ARM_REG_Q11, "q11"},
	{ ARM_REG_Q12, "q12"},
	{ ARM_REG_Q13, "q13"},
	{ ARM_REG_Q14, "q14"},
	{ ARM_REG_Q15, "q15"},
	{ ARM_REG_R0, "r0"},
	{ ARM_REG_R1, "r1"},
	{ ARM_REG_R2, "r2"},
	{ ARM_REG_R3, "r3"},
	{ ARM_REG_R4, "r4"},
	{ ARM_REG_R5, "r5"},
	{ ARM_REG_R6, "r6"},
	{ ARM_REG_R7, "r7"},
	{ ARM_REG_R8, "r8"},
	{ ARM_REG_R9, "sb"},
	{ ARM_REG_R10, "sl"},
	{ ARM_REG_R11, "fp"},
	{ ARM_REG_R12, "ip"},
	{ ARM_REG_S0, "s0"},
	{ ARM_REG_S1, "s1"},
	{ ARM_REG_S2, "s2"},
	{ ARM_REG_S3, "s3"},
	{ ARM_REG_S4, "s4"},
	{ ARM_REG_S5, "s5"},
	{ ARM_REG_S6, "s6"},
	{ ARM_REG_S7, "s7"},
	{ ARM_REG_S8, "s8"},
	{ ARM_REG_S9, "s9"},
	{ ARM_REG_S10, "s10"},
	{ ARM_REG_S11, "s11"},
	{ ARM_REG_S12, "s12"},
	{ ARM_REG_S13, "s13"},
	{ ARM_REG_S14, "s14"},
	{ ARM_REG_S15, "s15"},
	{ ARM_REG_S16, "s16"},
	{ ARM_REG_S17, "s17"},
	{ ARM_REG_S18, "s18"},
	{ ARM_REG_S19, "s19"},
	{ ARM_REG_S20, "s20"},
	{ ARM_REG_S21, "s21"},
	{ ARM_REG_S22, "s22"},
	{ ARM_REG_S23, "s23"},
	{ ARM_REG_S24, "s24"},
	{ ARM_REG_S25, "s25"},
	{ ARM_REG_S26, "s26"},
	{ ARM_REG_S27, "s27"},
	{ ARM_REG_S28, "s28"},
	{ ARM_REG_S29, "s29"},
	{ ARM_REG_S30, "s30"},
	{ ARM_REG_S31, "s31"},
};
static const name_map reg_name_maps2[] = {
	{ ARM_REG_INVALID, NULL },
	{ ARM_REG_APSR, "apsr"},
	{ ARM_REG_APSR_NZCV, "apsr_nzcv"},
	{ ARM_REG_CPSR, "cpsr"},
	{ ARM_REG_FPEXC, "fpexc"},
	{ ARM_REG_FPINST, "fpinst"},
	{ ARM_REG_FPSCR, "fpscr"},
	{ ARM_REG_FPSCR_NZCV, "fpscr_nzcv"},
	{ ARM_REG_FPSID, "fpsid"},
	{ ARM_REG_ITSTATE, "itstate"},
	{ ARM_REG_LR, "lr"},
	{ ARM_REG_PC, "pc"},
	{ ARM_REG_SP, "sp"},
	{ ARM_REG_SPSR, "spsr"},
	{ ARM_REG_D0, "d0"},
	{ ARM_REG_D1, "d1"},
	{ ARM_REG_D2, "d2"},
	{ ARM_REG_D3, "d3"},
	{ ARM_REG_D4, "d4"},
	{ ARM_REG_D5, "d5"},
	{ ARM_REG_D6, "d6"},
	{ ARM_REG_D7, "d7"},
	{ ARM_REG_D8, "d8"},
	{ ARM_REG_D9, "d9"},
	{ ARM_REG_D10, "d10"},
	{ ARM_REG_D11, "d11"},
	{ ARM_REG_D12, "d12"},
	{ ARM_REG_D13, "d13"},
	{ ARM_REG_D14, "d14"},
	{ ARM_REG_D15, "d15"},
	{ ARM_REG_D16, "d16"},
	{ ARM_REG_D17, "d17"},
	{ ARM_REG_D18, "d18"},
	{ ARM_REG_D19, "d19"},
	{ ARM_REG_D20, "d20"},
	{ ARM_REG_D21, "d21"},
	{ ARM_REG_D22, "d22"},
	{ ARM_REG_D23, "d23"},
	{ ARM_REG_D24, "d24"},
	{ ARM_REG_D25, "d25"},
	{ ARM_REG_D26, "d26"},
	{ ARM_REG_D27, "d27"},
	{ ARM_REG_D28, "d28"},
	{ ARM_REG_D29, "d29"},
	{ ARM_REG_D30, "d30"},
	{ ARM_REG_D31, "d31"},
	{ ARM_REG_FPINST2, "fpinst2"},
	{ ARM_REG_MVFR0, "mvfr0"},
	{ ARM_REG_MVFR1, "mvfr1"},
	{ ARM_REG_MVFR2, "mvfr2"},
	{ ARM_REG_Q0, "q0"},
	{ ARM_REG_Q1, "q1"},
	{ ARM_REG_Q2, "q2"},
	{ ARM_REG_Q3, "q3"},
	{ ARM_REG_Q4, "q4"},
	{ ARM_REG_Q5, "q5"},
	{ ARM_REG_Q6, "q6"},
	{ ARM_REG_Q7, "q7"},
	{ ARM_REG_Q8, "q8"},
	{ ARM_REG_Q9, "q9"},
	{ ARM_REG_Q10, "q10"},
	{ ARM_REG_Q11, "q11"},
	{ ARM_REG_Q12, "q12"},
	{ ARM_REG_Q13, "q13"},
	{ ARM_REG_Q14, "q14"},
	{ ARM_REG_Q15, "q15"},
	{ ARM_REG_R0, "r0"},
	{ ARM_REG_R1, "r1"},
	{ ARM_REG_R2, "r2"},
	{ ARM_REG_R3, "r3"},
	{ ARM_REG_R4, "r4"},
	{ ARM_REG_R5, "r5"},
	{ ARM_REG_R6, "r6"},
	{ ARM_REG_R7, "r7"},
	{ ARM_REG_R8, "r8"},
	{ ARM_REG_R9, "r9"},
	{ ARM_REG_R10, "r10"},
	{ ARM_REG_R11, "r11"},
	{ ARM_REG_R12, "r12"},
	{ ARM_REG_S0, "s0"},
	{ ARM_REG_S1, "s1"},
	{ ARM_REG_S2, "s2"},
	{ ARM_REG_S3, "s3"},
	{ ARM_REG_S4, "s4"},
	{ ARM_REG_S5, "s5"},
	{ ARM_REG_S6, "s6"},
	{ ARM_REG_S7, "s7"},
	{ ARM_REG_S8, "s8"},
	{ ARM_REG_S9, "s9"},
	{ ARM_REG_S10, "s10"},
	{ ARM_REG_S11, "s11"},
	{ ARM_REG_S12, "s12"},
	{ ARM_REG_S13, "s13"},
	{ ARM_REG_S14, "s14"},
	{ ARM_REG_S15, "s15"},
	{ ARM_REG_S16, "s16"},
	{ ARM_REG_S17, "s17"},
	{ ARM_REG_S18, "s18"},
	{ ARM_REG_S19, "s19"},
	{ ARM_REG_S20, "s20"},
	{ ARM_REG_S21, "s21"},
	{ ARM_REG_S22, "s22"},
	{ ARM_REG_S23, "s23"},
	{ ARM_REG_S24, "s24"},
	{ ARM_REG_S25, "s25"},
	{ ARM_REG_S26, "s26"},
	{ ARM_REG_S27, "s27"},
	{ ARM_REG_S28, "s28"},
	{ ARM_REG_S29, "s29"},
	{ ARM_REG_S30, "s30"},
	{ ARM_REG_S31, "s31"},
};
#endif

const char *ARM_reg_name(csh handle, unsigned int reg)
{
#ifndef CAPSTONE_DIET
	if (reg >= ARR_SIZE(reg_name_maps))
		return NULL;

	return reg_name_maps[reg].name;
#else
	return NULL;
#endif
}

const char *ARM_reg_name2(csh handle, unsigned int reg)
{
#ifndef CAPSTONE_DIET
	if (reg >= ARR_SIZE(reg_name_maps2))
		return NULL;

	return reg_name_maps2[reg].name;
#else
	return NULL;
#endif
}

static const insn_map insns[] = {
	// dummy item
	{
		0, 0,
#ifndef CAPSTONE_DIET
		{ 0 }, { 0 }, { 0 }, 0, 0
#endif
	},

#include "ARMMappingInsn.inc"
};

void ARM_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id)
{
	int i = insn_find(insns, ARR_SIZE(insns), id, &h->insn_cache);
	//printf(">> id = %u\n", id);
	if (i != 0) {
		insn->id = insns[i].mapid;

		if (h->detail) {
#ifndef CAPSTONE_DIET
			cs_struct handle;
			handle.detail = h->detail;

			memcpy(insn->detail->regs_read, insns[i].regs_use, sizeof(insns[i].regs_use));
			insn->detail->regs_read_count = (uint8_t)count_positive(insns[i].regs_use);

			memcpy(insn->detail->regs_write, insns[i].regs_mod, sizeof(insns[i].regs_mod));
			insn->detail->regs_write_count = (uint8_t)count_positive(insns[i].regs_mod);

			memcpy(insn->detail->groups, insns[i].groups, sizeof(insns[i].groups));
			insn->detail->groups_count = (uint8_t)count_positive8(insns[i].groups);

			insn->detail->arm.update_flags = cs_reg_write((csh)&handle, insn, ARM_REG_CPSR);

			if (insns[i].branch || insns[i].indirect_branch) {
				// this insn also belongs to JUMP group. add JUMP group
				insn->detail->groups[insn->detail->groups_count] = ARM_GRP_JUMP;
				insn->detail->groups_count++;
			}
#endif
		}
	}
}

#ifndef CAPSTONE_DIET
static const name_map insn_name_maps[] = {
	{ ARM_INS_INVALID, NULL },

	{ ARM_INS_ADC, "adc" },
	{ ARM_INS_ADD, "add" },
	{ ARM_INS_ADR, "adr" },
	{ ARM_INS_AESD, "aesd" },
	{ ARM_INS_AESE, "aese" },
	{ ARM_INS_AESIMC, "aesimc" },
	{ ARM_INS_AESMC, "aesmc" },
	{ ARM_INS_AND, "and" },
	{ ARM_INS_BFC, "bfc" },
	{ ARM_INS_BFI, "bfi" },
	{ ARM_INS_BIC, "bic" },
	{ ARM_INS_BKPT, "bkpt" },
	{ ARM_INS_BL, "bl" },
	{ ARM_INS_BLX, "blx" },
	{ ARM_INS_BX, "bx" },
	{ ARM_INS_BXJ, "bxj" },
	{ ARM_INS_B, "b" },
	{ ARM_INS_CDP, "cdp" },
	{ ARM_INS_CDP2, "cdp2" },
	{ ARM_INS_CLREX, "clrex" },
	{ ARM_INS_CLZ, "clz" },
	{ ARM_INS_CMN, "cmn" },
	{ ARM_INS_CMP, "cmp" },
	{ ARM_INS_CPS, "cps" },
	{ ARM_INS_CRC32B, "crc32b" },
	{ ARM_INS_CRC32CB, "crc32cb" },
	{ ARM_INS_CRC32CH, "crc32ch" },
	{ ARM_INS_CRC32CW, "crc32cw" },
	{ ARM_INS_CRC32H, "crc32h" },
	{ ARM_INS_CRC32W, "crc32w" },
	{ ARM_INS_DBG, "dbg" },
	{ ARM_INS_DMB, "dmb" },
	{ ARM_INS_DSB, "dsb" },
	{ ARM_INS_EOR, "eor" },
	{ ARM_INS_ERET, "eret" },
	{ ARM_INS_VMOV, "vmov" },
	{ ARM_INS_FLDMDBX, "fldmdbx" },
	{ ARM_INS_FLDMIAX, "fldmiax" },
	{ ARM_INS_VMRS, "vmrs" },
	{ ARM_INS_FSTMDBX, "fstmdbx" },
	{ ARM_INS_FSTMIAX, "fstmiax" },
	{ ARM_INS_HINT, "hint" },
	{ ARM_INS_HLT, "hlt" },
	{ ARM_INS_HVC, "hvc" },
	{ ARM_INS_ISB, "isb" },
	{ ARM_INS_LDA, "lda" },
	{ ARM_INS_LDAB, "ldab" },
	{ ARM_INS_LDAEX, "ldaex" },
	{ ARM_INS_LDAEXB, "ldaexb" },
	{ ARM_INS_LDAEXD, "ldaexd" },
	{ ARM_INS_LDAEXH, "ldaexh" },
	{ ARM_INS_LDAH, "ldah" },
	{ ARM_INS_LDC2L, "ldc2l" },
	{ ARM_INS_LDC2, "ldc2" },
	{ ARM_INS_LDCL, "ldcl" },
	{ ARM_INS_LDC, "ldc" },
	{ ARM_INS_LDMDA, "ldmda" },
	{ ARM_INS_LDMDB, "ldmdb" },
	{ ARM_INS_LDM, "ldm" },
	{ ARM_INS_LDMIB, "ldmib" },
	{ ARM_INS_LDRBT, "ldrbt" },
	{ ARM_INS_LDRB, "ldrb" },
	{ ARM_INS_LDRD, "ldrd" },
	{ ARM_INS_LDREX, "ldrex" },
	{ ARM_INS_LDREXB, "ldrexb" },
	{ ARM_INS_LDREXD, "ldrexd" },
	{ ARM_INS_LDREXH, "ldrexh" },
	{ ARM_INS_LDRH, "ldrh" },
	{ ARM_INS_LDRHT, "ldrht" },
	{ ARM_INS_LDRSB, "ldrsb" },
	{ ARM_INS_LDRSBT, "ldrsbt" },
	{ ARM_INS_LDRSH, "ldrsh" },
	{ ARM_INS_LDRSHT, "ldrsht" },
	{ ARM_INS_LDRT, "ldrt" },
	{ ARM_INS_LDR, "ldr" },
	{ ARM_INS_MCR, "mcr" },
	{ ARM_INS_MCR2, "mcr2" },
	{ ARM_INS_MCRR, "mcrr" },
	{ ARM_INS_MCRR2, "mcrr2" },
	{ ARM_INS_MLA, "mla" },
	{ ARM_INS_MLS, "mls" },
	{ ARM_INS_MOV, "mov" },
	{ ARM_INS_MOVT, "movt" },
	{ ARM_INS_MOVW, "movw" },
	{ ARM_INS_MRC, "mrc" },
	{ ARM_INS_MRC2, "mrc2" },
	{ ARM_INS_MRRC, "mrrc" },
	{ ARM_INS_MRRC2, "mrrc2" },
	{ ARM_INS_MRS, "mrs" },
	{ ARM_INS_MSR, "msr" },
	{ ARM_INS_MUL, "mul" },
	{ ARM_INS_MVN, "mvn" },
	{ ARM_INS_ORR, "orr" },
	{ ARM_INS_PKHBT, "pkhbt" },
	{ ARM_INS_PKHTB, "pkhtb" },
	{ ARM_INS_PLDW, "pldw" },
	{ ARM_INS_PLD, "pld" },
	{ ARM_INS_PLI, "pli" },
	{ ARM_INS_QADD, "qadd" },
	{ ARM_INS_QADD16, "qadd16" },
	{ ARM_INS_QADD8, "qadd8" },
	{ ARM_INS_QASX, "qasx" },
	{ ARM_INS_QDADD, "qdadd" },
	{ ARM_INS_QDSUB, "qdsub" },
	{ ARM_INS_QSAX, "qsax" },
	{ ARM_INS_QSUB, "qsub" },
	{ ARM_INS_QSUB16, "qsub16" },
	{ ARM_INS_QSUB8, "qsub8" },
	{ ARM_INS_RBIT, "rbit" },
	{ ARM_INS_REV, "rev" },
	{ ARM_INS_REV16, "rev16" },
	{ ARM_INS_REVSH, "revsh" },
	{ ARM_INS_RFEDA, "rfeda" },
	{ ARM_INS_RFEDB, "rfedb" },
	{ ARM_INS_RFEIA, "rfeia" },
	{ ARM_INS_RFEIB, "rfeib" },
	{ ARM_INS_RSB, "rsb" },
	{ ARM_INS_RSC, "rsc" },
	{ ARM_INS_SADD16, "sadd16" },
	{ ARM_INS_SADD8, "sadd8" },
	{ ARM_INS_SASX, "sasx" },
	{ ARM_INS_SBC, "sbc" },
	{ ARM_INS_SBFX, "sbfx" },
	{ ARM_INS_SDIV, "sdiv" },
	{ ARM_INS_SEL, "sel" },
	{ ARM_INS_SETEND, "setend" },
	{ ARM_INS_SHA1C, "sha1c" },
	{ ARM_INS_SHA1H, "sha1h" },
	{ ARM_INS_SHA1M, "sha1m" },
	{ ARM_INS_SHA1P, "sha1p" },
	{ ARM_INS_SHA1SU0, "sha1su0" },
	{ ARM_INS_SHA1SU1, "sha1su1" },
	{ ARM_INS_SHA256H, "sha256h" },
	{ ARM_INS_SHA256H2, "sha256h2" },
	{ ARM_INS_SHA256SU0, "sha256su0" },
	{ ARM_INS_SHA256SU1, "sha256su1" },
	{ ARM_INS_SHADD16, "shadd16" },
	{ ARM_INS_SHADD8, "shadd8" },
	{ ARM_INS_SHASX, "shasx" },
	{ ARM_INS_SHSAX, "shsax" },
	{ ARM_INS_SHSUB16, "shsub16" },
	{ ARM_INS_SHSUB8, "shsub8" },
	{ ARM_INS_SMC, "smc" },
	{ ARM_INS_SMLABB, "smlabb" },
	{ ARM_INS_SMLABT, "smlabt" },
	{ ARM_INS_SMLAD, "smlad" },
	{ ARM_INS_SMLADX, "smladx" },
	{ ARM_INS_SMLAL, "smlal" },
	{ ARM_INS_SMLALBB, "smlalbb" },
	{ ARM_INS_SMLALBT, "smlalbt" },
	{ ARM_INS_SMLALD, "smlald" },
	{ ARM_INS_SMLALDX, "smlaldx" },
	{ ARM_INS_SMLALTB, "smlaltb" },
	{ ARM_INS_SMLALTT, "smlaltt" },
	{ ARM_INS_SMLATB, "smlatb" },
	{ ARM_INS_SMLATT, "smlatt" },
	{ ARM_INS_SMLAWB, "smlawb" },
	{ ARM_INS_SMLAWT, "smlawt" },
	{ ARM_INS_SMLSD, "smlsd" },
	{ ARM_INS_SMLSDX, "smlsdx" },
	{ ARM_INS_SMLSLD, "smlsld" },
	{ ARM_INS_SMLSLDX, "smlsldx" },
	{ ARM_INS_SMMLA, "smmla" },
	{ ARM_INS_SMMLAR, "smmlar" },
	{ ARM_INS_SMMLS, "smmls" },
	{ ARM_INS_SMMLSR, "smmlsr" },
	{ ARM_INS_SMMUL, "smmul" },
	{ ARM_INS_SMMULR, "smmulr" },
	{ ARM_INS_SMUAD, "smuad" },
	{ ARM_INS_SMUADX, "smuadx" },
	{ ARM_INS_SMULBB, "smulbb" },
	{ ARM_INS_SMULBT, "smulbt" },
	{ ARM_INS_SMULL, "smull" },
	{ ARM_INS_SMULTB, "smultb" },
	{ ARM_INS_SMULTT, "smultt" },
	{ ARM_INS_SMULWB, "smulwb" },
	{ ARM_INS_SMULWT, "smulwt" },
	{ ARM_INS_SMUSD, "smusd" },
	{ ARM_INS_SMUSDX, "smusdx" },
	{ ARM_INS_SRSDA, "srsda" },
	{ ARM_INS_SRSDB, "srsdb" },
	{ ARM_INS_SRSIA, "srsia" },
	{ ARM_INS_SRSIB, "srsib" },
	{ ARM_INS_SSAT, "ssat" },
	{ ARM_INS_SSAT16, "ssat16" },
	{ ARM_INS_SSAX, "ssax" },
	{ ARM_INS_SSUB16, "ssub16" },
	{ ARM_INS_SSUB8, "ssub8" },
	{ ARM_INS_STC2L, "stc2l" },
	{ ARM_INS_STC2, "stc2" },
	{ ARM_INS_STCL, "stcl" },
	{ ARM_INS_STC, "stc" },
	{ ARM_INS_STL, "stl" },
	{ ARM_INS_STLB, "stlb" },
	{ ARM_INS_STLEX, "stlex" },
	{ ARM_INS_STLEXB, "stlexb" },
	{ ARM_INS_STLEXD, "stlexd" },
	{ ARM_INS_STLEXH, "stlexh" },
	{ ARM_INS_STLH, "stlh" },
	{ ARM_INS_STMDA, "stmda" },
	{ ARM_INS_STMDB, "stmdb" },
	{ ARM_INS_STM, "stm" },
	{ ARM_INS_STMIB, "stmib" },
	{ ARM_INS_STRBT, "strbt" },
	{ ARM_INS_STRB, "strb" },
	{ ARM_INS_STRD, "strd" },
	{ ARM_INS_STREX, "strex" },
	{ ARM_INS_STREXB, "strexb" },
	{ ARM_INS_STREXD, "strexd" },
	{ ARM_INS_STREXH, "strexh" },
	{ ARM_INS_STRH, "strh" },
	{ ARM_INS_STRHT, "strht" },
	{ ARM_INS_STRT, "strt" },
	{ ARM_INS_STR, "str" },
	{ ARM_INS_SUB, "sub" },
	{ ARM_INS_SVC, "svc" },
	{ ARM_INS_SWP, "swp" },
	{ ARM_INS_SWPB, "swpb" },
	{ ARM_INS_SXTAB, "sxtab" },
	{ ARM_INS_SXTAB16, "sxtab16" },
	{ ARM_INS_SXTAH, "sxtah" },
	{ ARM_INS_SXTB, "sxtb" },
	{ ARM_INS_SXTB16, "sxtb16" },
	{ ARM_INS_SXTH, "sxth" },
	{ ARM_INS_TEQ, "teq" },
	{ ARM_INS_TRAP, "trap" },
	{ ARM_INS_TST, "tst" },
	{ ARM_INS_UADD16, "uadd16" },
	{ ARM_INS_UADD8, "uadd8" },
	{ ARM_INS_UASX, "uasx" },
	{ ARM_INS_UBFX, "ubfx" },
	{ ARM_INS_UDF, "udf" },
	{ ARM_INS_UDIV, "udiv" },
	{ ARM_INS_UHADD16, "uhadd16" },
	{ ARM_INS_UHADD8, "uhadd8" },
	{ ARM_INS_UHASX, "uhasx" },
	{ ARM_INS_UHSAX, "uhsax" },
	{ ARM_INS_UHSUB16, "uhsub16" },
	{ ARM_INS_UHSUB8, "uhsub8" },
	{ ARM_INS_UMAAL, "umaal" },
	{ ARM_INS_UMLAL, "umlal" },
	{ ARM_INS_UMULL, "umull" },
	{ ARM_INS_UQADD16, "uqadd16" },
	{ ARM_INS_UQADD8, "uqadd8" },
	{ ARM_INS_UQASX, "uqasx" },
	{ ARM_INS_UQSAX, "uqsax" },
	{ ARM_INS_UQSUB16, "uqsub16" },
	{ ARM_INS_UQSUB8, "uqsub8" },
	{ ARM_INS_USAD8, "usad8" },
	{ ARM_INS_USADA8, "usada8" },
	{ ARM_INS_USAT, "usat" },
	{ ARM_INS_USAT16, "usat16" },
	{ ARM_INS_USAX, "usax" },
	{ ARM_INS_USUB16, "usub16" },
	{ ARM_INS_USUB8, "usub8" },
	{ ARM_INS_UXTAB, "uxtab" },
	{ ARM_INS_UXTAB16, "uxtab16" },
	{ ARM_INS_UXTAH, "uxtah" },
	{ ARM_INS_UXTB, "uxtb" },
	{ ARM_INS_UXTB16, "uxtb16" },
	{ ARM_INS_UXTH, "uxth" },
	{ ARM_INS_VABAL, "vabal" },
	{ ARM_INS_VABA, "vaba" },
	{ ARM_INS_VABDL, "vabdl" },
	{ ARM_INS_VABD, "vabd" },
	{ ARM_INS_VABS, "vabs" },
	{ ARM_INS_VACGE, "vacge" },
	{ ARM_INS_VACGT, "vacgt" },
	{ ARM_INS_VADD, "vadd" },
	{ ARM_INS_VADDHN, "vaddhn" },
	{ ARM_INS_VADDL, "vaddl" },
	{ ARM_INS_VADDW, "vaddw" },
	{ ARM_INS_VAND, "vand" },
	{ ARM_INS_VBIC, "vbic" },
	{ ARM_INS_VBIF, "vbif" },
	{ ARM_INS_VBIT, "vbit" },
	{ ARM_INS_VBSL, "vbsl" },
	{ ARM_INS_VCEQ, "vceq" },
	{ ARM_INS_VCGE, "vcge" },
	{ ARM_INS_VCGT, "vcgt" },
	{ ARM_INS_VCLE, "vcle" },
	{ ARM_INS_VCLS, "vcls" },
	{ ARM_INS_VCLT, "vclt" },
	{ ARM_INS_VCLZ, "vclz" },
	{ ARM_INS_VCMP, "vcmp" },
	{ ARM_INS_VCMPE, "vcmpe" },
	{ ARM_INS_VCNT, "vcnt" },
	{ ARM_INS_VCVTA, "vcvta" },
	{ ARM_INS_VCVTB, "vcvtb" },
	{ ARM_INS_VCVT, "vcvt" },
	{ ARM_INS_VCVTM, "vcvtm" },
	{ ARM_INS_VCVTN, "vcvtn" },
	{ ARM_INS_VCVTP, "vcvtp" },
	{ ARM_INS_VCVTT, "vcvtt" },
	{ ARM_INS_VDIV, "vdiv" },
	{ ARM_INS_VDUP, "vdup" },
	{ ARM_INS_VEOR, "veor" },
	{ ARM_INS_VEXT, "vext" },
	{ ARM_INS_VFMA, "vfma" },
	{ ARM_INS_VFMS, "vfms" },
	{ ARM_INS_VFNMA, "vfnma" },
	{ ARM_INS_VFNMS, "vfnms" },
	{ ARM_INS_VHADD, "vhadd" },
	{ ARM_INS_VHSUB, "vhsub" },
	{ ARM_INS_VLD1, "vld1" },
	{ ARM_INS_VLD2, "vld2" },
	{ ARM_INS_VLD3, "vld3" },
	{ ARM_INS_VLD4, "vld4" },
	{ ARM_INS_VLDMDB, "vldmdb" },
	{ ARM_INS_VLDMIA, "vldmia" },
	{ ARM_INS_VLDR, "vldr" },
	{ ARM_INS_VMAXNM, "vmaxnm" },
	{ ARM_INS_VMAX, "vmax" },
	{ ARM_INS_VMINNM, "vminnm" },
	{ ARM_INS_VMIN, "vmin" },
	{ ARM_INS_VMLA, "vmla" },
	{ ARM_INS_VMLAL, "vmlal" },
	{ ARM_INS_VMLS, "vmls" },
	{ ARM_INS_VMLSL, "vmlsl" },
	{ ARM_INS_VMOVL, "vmovl" },
	{ ARM_INS_VMOVN, "vmovn" },
	{ ARM_INS_VMSR, "vmsr" },
	{ ARM_INS_VMUL, "vmul" },
	{ ARM_INS_VMULL, "vmull" },
	{ ARM_INS_VMVN, "vmvn" },
	{ ARM_INS_VNEG, "vneg" },
	{ ARM_INS_VNMLA, "vnmla" },
	{ ARM_INS_VNMLS, "vnmls" },
	{ ARM_INS_VNMUL, "vnmul" },
	{ ARM_INS_VORN, "vorn" },
	{ ARM_INS_VORR, "vorr" },
	{ ARM_INS_VPADAL, "vpadal" },
	{ ARM_INS_VPADDL, "vpaddl" },
	{ ARM_INS_VPADD, "vpadd" },
	{ ARM_INS_VPMAX, "vpmax" },
	{ ARM_INS_VPMIN, "vpmin" },
	{ ARM_INS_VQABS, "vqabs" },
	{ ARM_INS_VQADD, "vqadd" },
	{ ARM_INS_VQDMLAL, "vqdmlal" },
	{ ARM_INS_VQDMLSL, "vqdmlsl" },
	{ ARM_INS_VQDMULH, "vqdmulh" },
	{ ARM_INS_VQDMULL, "vqdmull" },
	{ ARM_INS_VQMOVUN, "vqmovun" },
	{ ARM_INS_VQMOVN, "vqmovn" },
	{ ARM_INS_VQNEG, "vqneg" },
	{ ARM_INS_VQRDMULH, "vqrdmulh" },
	{ ARM_INS_VQRSHL, "vqrshl" },
	{ ARM_INS_VQRSHRN, "vqrshrn" },
	{ ARM_INS_VQRSHRUN, "vqrshrun" },
	{ ARM_INS_VQSHL, "vqshl" },
	{ ARM_INS_VQSHLU, "vqshlu" },
	{ ARM_INS_VQSHRN, "vqshrn" },
	{ ARM_INS_VQSHRUN, "vqshrun" },
	{ ARM_INS_VQSUB, "vqsub" },
	{ ARM_INS_VRADDHN, "vraddhn" },
	{ ARM_INS_VRECPE, "vrecpe" },
	{ ARM_INS_VRECPS, "vrecps" },
	{ ARM_INS_VREV16, "vrev16" },
	{ ARM_INS_VREV32, "vrev32" },
	{ ARM_INS_VREV64, "vrev64" },
	{ ARM_INS_VRHADD, "vrhadd" },
	{ ARM_INS_VRINTA, "vrinta" },
	{ ARM_INS_VRINTM, "vrintm" },
	{ ARM_INS_VRINTN, "vrintn" },
	{ ARM_INS_VRINTP, "vrintp" },
	{ ARM_INS_VRINTR, "vrintr" },
	{ ARM_INS_VRINTX, "vrintx" },
	{ ARM_INS_VRINTZ, "vrintz" },
	{ ARM_INS_VRSHL, "vrshl" },
	{ ARM_INS_VRSHRN, "vrshrn" },
	{ ARM_INS_VRSHR, "vrshr" },
	{ ARM_INS_VRSQRTE, "vrsqrte" },
	{ ARM_INS_VRSQRTS, "vrsqrts" },
	{ ARM_INS_VRSRA, "vrsra" },
	{ ARM_INS_VRSUBHN, "vrsubhn" },
	{ ARM_INS_VSELEQ, "vseleq" },
	{ ARM_INS_VSELGE, "vselge" },
	{ ARM_INS_VSELGT, "vselgt" },
	{ ARM_INS_VSELVS, "vselvs" },
	{ ARM_INS_VSHLL, "vshll" },
	{ ARM_INS_VSHL, "vshl" },
	{ ARM_INS_VSHRN, "vshrn" },
	{ ARM_INS_VSHR, "vshr" },
	{ ARM_INS_VSLI, "vsli" },
	{ ARM_INS_VSQRT, "vsqrt" },
	{ ARM_INS_VSRA, "vsra" },
	{ ARM_INS_VSRI, "vsri" },
	{ ARM_INS_VST1, "vst1" },
	{ ARM_INS_VST2, "vst2" },
	{ ARM_INS_VST3, "vst3" },
	{ ARM_INS_VST4, "vst4" },
	{ ARM_INS_VSTMDB, "vstmdb" },
	{ ARM_INS_VSTMIA, "vstmia" },
	{ ARM_INS_VSTR, "vstr" },
	{ ARM_INS_VSUB, "vsub" },
	{ ARM_INS_VSUBHN, "vsubhn" },
	{ ARM_INS_VSUBL, "vsubl" },
	{ ARM_INS_VSUBW, "vsubw" },
	{ ARM_INS_VSWP, "vswp" },
	{ ARM_INS_VTBL, "vtbl" },
	{ ARM_INS_VTBX, "vtbx" },
	{ ARM_INS_VCVTR, "vcvtr" },
	{ ARM_INS_VTRN, "vtrn" },
	{ ARM_INS_VTST, "vtst" },
	{ ARM_INS_VUZP, "vuzp" },
	{ ARM_INS_VZIP, "vzip" },
	{ ARM_INS_ADDW, "addw" },
	{ ARM_INS_ASR, "asr" },
	{ ARM_INS_DCPS1, "dcps1" },
	{ ARM_INS_DCPS2, "dcps2" },
	{ ARM_INS_DCPS3, "dcps3" },
	{ ARM_INS_IT, "it" },
	{ ARM_INS_LSL, "lsl" },
	{ ARM_INS_LSR, "lsr" },
	{ ARM_INS_ORN, "orn" },
	{ ARM_INS_ROR, "ror" },
	{ ARM_INS_RRX, "rrx" },
	{ ARM_INS_SUBW, "subw" },
	{ ARM_INS_TBB, "tbb" },
	{ ARM_INS_TBH, "tbh" },
	{ ARM_INS_CBNZ, "cbnz" },
	{ ARM_INS_CBZ, "cbz" },
	{ ARM_INS_POP, "pop" },
	{ ARM_INS_PUSH, "push" },

	// special instructions
	{ ARM_INS_NOP, "nop" },
	{ ARM_INS_YIELD, "yield" },
	{ ARM_INS_WFE, "wfe" },
	{ ARM_INS_WFI, "wfi" },
	{ ARM_INS_SEV, "sev" },
	{ ARM_INS_SEVL, "sevl" },
	{ ARM_INS_VPUSH, "vpush" },
	{ ARM_INS_VPOP, "vpop" },
};
#endif

const char *ARM_insn_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	if (id >= ARM_INS_ENDING)
		return NULL;

	return insn_name_maps[id].name;
#else
	return NULL;
#endif
}

#ifndef CAPSTONE_DIET
static const name_map group_name_maps[] = {
	// generic groups
	{ ARM_GRP_INVALID, NULL },
	{ ARM_GRP_JUMP,	"jump" },
	{ ARM_GRP_CALL,	"call" },
	{ ARM_GRP_INT,	"int" },
	{ ARM_GRP_PRIVILEGE, "privilege" },
	{ ARM_GRP_BRANCH_RELATIVE, "branch_relative" },

	// architecture-specific groups
	{ ARM_GRP_CRYPTO, "crypto" },
	{ ARM_GRP_DATABARRIER, "databarrier" },
	{ ARM_GRP_DIVIDE, "divide" },
	{ ARM_GRP_FPARMV8, "fparmv8" },
	{ ARM_GRP_MULTPRO, "multpro" },
	{ ARM_GRP_NEON, "neon" },
	{ ARM_GRP_T2EXTRACTPACK, "T2EXTRACTPACK" },
	{ ARM_GRP_THUMB2DSP, "THUMB2DSP" },
	{ ARM_GRP_TRUSTZONE, "TRUSTZONE" },
	{ ARM_GRP_V4T, "v4t" },
	{ ARM_GRP_V5T, "v5t" },
	{ ARM_GRP_V5TE, "v5te" },
	{ ARM_GRP_V6, "v6" },
	{ ARM_GRP_V6T2, "v6t2" },
	{ ARM_GRP_V7, "v7" },
	{ ARM_GRP_V8, "v8" },
	{ ARM_GRP_VFP2, "vfp2" },
	{ ARM_GRP_VFP3, "vfp3" },
	{ ARM_GRP_VFP4, "vfp4" },
	{ ARM_GRP_ARM, "arm" },
	{ ARM_GRP_MCLASS, "mclass" },
	{ ARM_GRP_NOTMCLASS, "notmclass" },
	{ ARM_GRP_THUMB, "thumb" },
	{ ARM_GRP_THUMB1ONLY, "thumb1only" },
	{ ARM_GRP_THUMB2, "thumb2" },
	{ ARM_GRP_PREV8, "prev8" },
	{ ARM_GRP_FPVMLX, "fpvmlx" },
	{ ARM_GRP_MULOPS, "mulops" },
	{ ARM_GRP_CRC, "crc" },
	{ ARM_GRP_DPVFP, "dpvfp" },
	{ ARM_GRP_V6M, "v6m" },
	{ ARM_GRP_VIRTUALIZATION, "virtualization" },
};
#endif

const char *ARM_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(group_name_maps, ARR_SIZE(group_name_maps), id);
#else
	return NULL;
#endif
}

// list all relative branch instructions
// ie: insns[i].branch && !insns[i].indirect_branch
static const unsigned int insn_rel[] = {
	ARM_BL,
	ARM_BLX_pred,
	ARM_Bcc,
	ARM_t2B,
	ARM_t2Bcc,
	ARM_tB,
	ARM_tBcc,
	ARM_tCBNZ,
	ARM_tCBZ,
	ARM_BL_pred,
	ARM_BLXi,
	ARM_tBL,
	ARM_tBLXi,
	0
};

static const unsigned int insn_blx_rel_to_arm[] = {
	ARM_tBLXi,
	0
};

// check if this insn is relative branch
bool ARM_rel_branch(cs_struct *h, unsigned int id)
{
	int i;

	for (i = 0; insn_rel[i]; i++) {
		if (id == insn_rel[i]) {
			return true;
		}
	}

	// not found
	return false;
}

bool ARM_blx_to_arm_mode(cs_struct *h, unsigned int id) {
	int i;

	for (i = 0; insn_blx_rel_to_arm[i]; i++)
		if (id == insn_blx_rel_to_arm[i])
			return true;

	// not found
	return false;

}

#ifndef CAPSTONE_DIET
// map instruction to its characteristics
typedef struct insn_op {
	uint8_t access[7];
} insn_op;

static insn_op insn_ops[] = {
	{
		// NULL item
		{ 0 }
	},

#include "ARMMappingInsnOp.inc"
};

// given internal insn id, return operand access info
uint8_t *ARM_get_op_access(cs_struct *h, unsigned int id)
{
	int i = insn_find(insns, ARR_SIZE(insns), id, &h->insn_cache);
	if (i != 0) {
		return insn_ops[i].access;
	}

	return NULL;
}

void ARM_reg_access(const cs_insn *insn,
		cs_regs regs_read, uint8_t *regs_read_count,
		cs_regs regs_write, uint8_t *regs_write_count)
{
	uint8_t i;
	uint8_t read_count, write_count;
	cs_arm *arm = &(insn->detail->arm);

	read_count = insn->detail->regs_read_count;
	write_count = insn->detail->regs_write_count;

	// implicit registers
	memcpy(regs_read, insn->detail->regs_read, read_count * sizeof(insn->detail->regs_read[0]));
	memcpy(regs_write, insn->detail->regs_write, write_count * sizeof(insn->detail->regs_write[0]));

	// explicit registers
	for (i = 0; i < arm->op_count; i++) {
		cs_arm_op *op = &(arm->operands[i]);
		switch((int)op->type) {
			case ARM_OP_REG:
				if ((op->access & CS_AC_READ) && !arr_exist(regs_read, read_count, op->reg)) {
					regs_read[read_count] = (uint16_t)op->reg;
					read_count++;
				}
				if ((op->access & CS_AC_WRITE) && !arr_exist(regs_write, write_count, op->reg)) {
					regs_write[write_count] = (uint16_t)op->reg;
					write_count++;
				}
				break;
			case ARM_OP_MEM:
				// registers appeared in memory references always being read
				if ((op->mem.base != ARM_REG_INVALID) && !arr_exist(regs_read, read_count, op->mem.base)) {
					regs_read[read_count] = (uint16_t)op->mem.base;
					read_count++;
				}
				if ((op->mem.index != ARM_REG_INVALID) && !arr_exist(regs_read, read_count, op->mem.index)) {
					regs_read[read_count] = (uint16_t)op->mem.index;
					read_count++;
				}
				if ((arm->writeback) && (op->mem.base != ARM_REG_INVALID) && !arr_exist(regs_write, write_count, op->mem.base)) {
					regs_write[write_count] = (uint16_t)op->mem.base;
					write_count++;
				}
			default:
				break;
		}
	}

	*regs_read_count = read_count;
	*regs_write_count = write_count;
}
#endif

#endif
