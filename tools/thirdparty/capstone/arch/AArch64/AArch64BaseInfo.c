//===-- AArch64BaseInfo.cpp - AArch64 Base encoding information------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides basic encoding and assembly information for AArch64.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_ARM64

#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#pragma warning(disable:4996)			// disable MSVC's warning on strcpy()
#pragma warning(disable:28719)		// disable MSVC's warning on strcpy()
#endif

#include "../../utils.h"

#include <stdio.h>
#include <stdlib.h>

#include "AArch64BaseInfo.h"

const char *A64NamedImmMapper_toString(const A64NamedImmMapper *N, uint32_t Value, bool *Valid)
{
	unsigned i;
	for (i = 0; i < N->NumPairs; ++i) {
		if (N->Pairs[i].Value == Value) {
			*Valid = true;
			return N->Pairs[i].Name;
		}
	}

	*Valid = false;
	return 0;
}

// compare s1 with lower(s2)
// return true if s1 == lower(f2), and false otherwise
static bool compare_lower_str(const char *s1, const char *s2)
{
	bool res;
	char *lower = cs_strdup(s2), *c;
	for (c = lower; *c; c++)
		*c = (char)tolower((int) *c);

	res = (strcmp(s1, lower) == 0);
	cs_mem_free(lower);

	return res;
}

uint32_t A64NamedImmMapper_fromString(const A64NamedImmMapper *N, char *Name, bool *Valid)
{
	unsigned i;
	for (i = 0; i < N->NumPairs; ++i) {
		if (compare_lower_str(N->Pairs[i].Name, Name)) {
			*Valid = true;
			return N->Pairs[i].Value;
		}
	}

	*Valid = false;
	return (uint32_t)-1;
}

bool A64NamedImmMapper_validImm(const A64NamedImmMapper *N, uint32_t Value)
{
	return Value < N->TooBigImm;
}

// return a string representing the number X
// NOTE: caller must free() the result itself to avoid memory leak
static char *utostr(uint64_t X, bool isNeg)
{
	char Buffer[22];
	char *BufPtr = Buffer+21;
	char *result;

	Buffer[21] = '\0';
	if (X == 0) *--BufPtr = '0';  // Handle special case...

	while (X) {
		*--BufPtr = X % 10 + '0';
		X /= 10;
	}

	if (isNeg) *--BufPtr = '-';   // Add negative sign...

	result = cs_strdup(BufPtr);
	return result;
}

static const A64NamedImmMapper_Mapping SysRegPairs[] = {
	{"pan", A64SysReg_PAN},
	{"uao", A64SysReg_UAO},
	{"osdtrrx_el1", A64SysReg_OSDTRRX_EL1},
	{"osdtrtx_el1",  A64SysReg_OSDTRTX_EL1},
	{"teecr32_el1", A64SysReg_TEECR32_EL1},
	{"mdccint_el1", A64SysReg_MDCCINT_EL1},
	{"mdscr_el1", A64SysReg_MDSCR_EL1},
	{"dbgdtr_el0", A64SysReg_DBGDTR_EL0},
	{"oseccr_el1", A64SysReg_OSECCR_EL1},
	{"dbgvcr32_el2", A64SysReg_DBGVCR32_EL2},
	{"dbgbvr0_el1", A64SysReg_DBGBVR0_EL1},
	{"dbgbvr1_el1", A64SysReg_DBGBVR1_EL1},
	{"dbgbvr2_el1", A64SysReg_DBGBVR2_EL1},
	{"dbgbvr3_el1", A64SysReg_DBGBVR3_EL1},
	{"dbgbvr4_el1", A64SysReg_DBGBVR4_EL1},
	{"dbgbvr5_el1", A64SysReg_DBGBVR5_EL1},
	{"dbgbvr6_el1", A64SysReg_DBGBVR6_EL1},
	{"dbgbvr7_el1", A64SysReg_DBGBVR7_EL1},
	{"dbgbvr8_el1", A64SysReg_DBGBVR8_EL1},
	{"dbgbvr9_el1", A64SysReg_DBGBVR9_EL1},
	{"dbgbvr10_el1", A64SysReg_DBGBVR10_EL1},
	{"dbgbvr11_el1", A64SysReg_DBGBVR11_EL1},
	{"dbgbvr12_el1", A64SysReg_DBGBVR12_EL1},
	{"dbgbvr13_el1", A64SysReg_DBGBVR13_EL1},
	{"dbgbvr14_el1", A64SysReg_DBGBVR14_EL1},
	{"dbgbvr15_el1", A64SysReg_DBGBVR15_EL1},
	{"dbgbcr0_el1", A64SysReg_DBGBCR0_EL1},
	{"dbgbcr1_el1", A64SysReg_DBGBCR1_EL1},
	{"dbgbcr2_el1", A64SysReg_DBGBCR2_EL1},
	{"dbgbcr3_el1", A64SysReg_DBGBCR3_EL1},
	{"dbgbcr4_el1", A64SysReg_DBGBCR4_EL1},
	{"dbgbcr5_el1", A64SysReg_DBGBCR5_EL1},
	{"dbgbcr6_el1", A64SysReg_DBGBCR6_EL1},
	{"dbgbcr7_el1", A64SysReg_DBGBCR7_EL1},
	{"dbgbcr8_el1", A64SysReg_DBGBCR8_EL1},
	{"dbgbcr9_el1", A64SysReg_DBGBCR9_EL1},
	{"dbgbcr10_el1", A64SysReg_DBGBCR10_EL1},
	{"dbgbcr11_el1", A64SysReg_DBGBCR11_EL1},
	{"dbgbcr12_el1", A64SysReg_DBGBCR12_EL1},
	{"dbgbcr13_el1", A64SysReg_DBGBCR13_EL1},
	{"dbgbcr14_el1", A64SysReg_DBGBCR14_EL1},
	{"dbgbcr15_el1", A64SysReg_DBGBCR15_EL1},
	{"dbgwvr0_el1", A64SysReg_DBGWVR0_EL1},
	{"dbgwvr1_el1", A64SysReg_DBGWVR1_EL1},
	{"dbgwvr2_el1", A64SysReg_DBGWVR2_EL1},
	{"dbgwvr3_el1", A64SysReg_DBGWVR3_EL1},
	{"dbgwvr4_el1", A64SysReg_DBGWVR4_EL1},
	{"dbgwvr5_el1", A64SysReg_DBGWVR5_EL1},
	{"dbgwvr6_el1", A64SysReg_DBGWVR6_EL1},
	{"dbgwvr7_el1", A64SysReg_DBGWVR7_EL1},
	{"dbgwvr8_el1", A64SysReg_DBGWVR8_EL1},
	{"dbgwvr9_el1", A64SysReg_DBGWVR9_EL1},
	{"dbgwvr10_el1", A64SysReg_DBGWVR10_EL1},
	{"dbgwvr11_el1", A64SysReg_DBGWVR11_EL1},
	{"dbgwvr12_el1", A64SysReg_DBGWVR12_EL1},
	{"dbgwvr13_el1", A64SysReg_DBGWVR13_EL1},
	{"dbgwvr14_el1", A64SysReg_DBGWVR14_EL1},
	{"dbgwvr15_el1", A64SysReg_DBGWVR15_EL1},
	{"dbgwcr0_el1", A64SysReg_DBGWCR0_EL1},
	{"dbgwcr1_el1", A64SysReg_DBGWCR1_EL1},
	{"dbgwcr2_el1", A64SysReg_DBGWCR2_EL1},
	{"dbgwcr3_el1", A64SysReg_DBGWCR3_EL1},
	{"dbgwcr4_el1", A64SysReg_DBGWCR4_EL1},
	{"dbgwcr5_el1", A64SysReg_DBGWCR5_EL1},
	{"dbgwcr6_el1", A64SysReg_DBGWCR6_EL1},
	{"dbgwcr7_el1", A64SysReg_DBGWCR7_EL1},
	{"dbgwcr8_el1", A64SysReg_DBGWCR8_EL1},
	{"dbgwcr9_el1", A64SysReg_DBGWCR9_EL1},
	{"dbgwcr10_el1", A64SysReg_DBGWCR10_EL1},
	{"dbgwcr11_el1", A64SysReg_DBGWCR11_EL1},
	{"dbgwcr12_el1", A64SysReg_DBGWCR12_EL1},
	{"dbgwcr13_el1", A64SysReg_DBGWCR13_EL1},
	{"dbgwcr14_el1", A64SysReg_DBGWCR14_EL1},
	{"dbgwcr15_el1", A64SysReg_DBGWCR15_EL1},
	{"teehbr32_el1", A64SysReg_TEEHBR32_EL1},
	{"osdlr_el1", A64SysReg_OSDLR_EL1},
	{"dbgprcr_el1", A64SysReg_DBGPRCR_EL1},
	{"dbgclaimset_el1", A64SysReg_DBGCLAIMSET_EL1},
	{"dbgclaimclr_el1", A64SysReg_DBGCLAIMCLR_EL1},
	{"csselr_el1", A64SysReg_CSSELR_EL1},
	{"vpidr_el2", A64SysReg_VPIDR_EL2},
	{"vmpidr_el2", A64SysReg_VMPIDR_EL2},
	{"sctlr_el1", A64SysReg_SCTLR_EL1},
	{"sctlr_el12", A64SysReg_SCTLR_EL12},
	{"sctlr_el2", A64SysReg_SCTLR_EL2},
	{"sctlr_el3", A64SysReg_SCTLR_EL3},
	{"actlr_el1", A64SysReg_ACTLR_EL1},
	{"actlr_el2", A64SysReg_ACTLR_EL2},
	{"actlr_el3", A64SysReg_ACTLR_EL3},
	{"cpacr_el1", A64SysReg_CPACR_EL1},
	{"cpacr_el12", A64SysReg_CPACR_EL12},
	{"hcr_el2", A64SysReg_HCR_EL2},
	{"scr_el3", A64SysReg_SCR_EL3},
	{"mdcr_el2", A64SysReg_MDCR_EL2},
	{"sder32_el3", A64SysReg_SDER32_EL3},
	{"cptr_el2", A64SysReg_CPTR_EL2},
	{"cptr_el3", A64SysReg_CPTR_EL3},
	{"hstr_el2", A64SysReg_HSTR_EL2},
	{"hacr_el2", A64SysReg_HACR_EL2},
	{"mdcr_el3", A64SysReg_MDCR_EL3},
	{"ttbr0_el1", A64SysReg_TTBR0_EL1},
	{"ttbr0_el12", A64SysReg_TTBR0_EL12},
	{"ttbr0_el2", A64SysReg_TTBR0_EL2},
	{"ttbr0_el3", A64SysReg_TTBR0_EL3},
	{"ttbr1_el1", A64SysReg_TTBR1_EL1},
	{"ttbr1_el12", A64SysReg_TTBR1_EL12},
	{"ttbr1_el2", A64SysReg_TTBR1_EL2},
	{"tcr_el1", A64SysReg_TCR_EL1},
	{"tcr_el12", A64SysReg_TCR_EL12},
	{"tcr_el2", A64SysReg_TCR_EL2},
	{"tcr_el3", A64SysReg_TCR_EL3},
	{"vttbr_el2", A64SysReg_VTTBR_EL2},
	{"vtcr_el2", A64SysReg_VTCR_EL2},
	{"dacr32_el2", A64SysReg_DACR32_EL2},
	{"spsr_el1", A64SysReg_SPSR_EL1},
	{"spsr_el12", A64SysReg_SPSR_EL12},
	{"spsr_el2", A64SysReg_SPSR_EL2},
	{"spsr_el3", A64SysReg_SPSR_EL3},
	{"elr_el1", A64SysReg_ELR_EL1},
	{"elr_el12", A64SysReg_ELR_EL12},
	{"elr_el2", A64SysReg_ELR_EL2},
	{"elr_el3", A64SysReg_ELR_EL3},
	{"sp_el0", A64SysReg_SP_EL0},
	{"sp_el1", A64SysReg_SP_EL1},
	{"sp_el2", A64SysReg_SP_EL2},
	{"spsel", A64SysReg_SPSel},
	{"nzcv", A64SysReg_NZCV},
	{"daif", A64SysReg_DAIF},
	{"currentel", A64SysReg_CurrentEL},
	{"spsr_irq", A64SysReg_SPSR_irq},
	{"spsr_abt", A64SysReg_SPSR_abt},
	{"spsr_und", A64SysReg_SPSR_und},
	{"spsr_fiq", A64SysReg_SPSR_fiq},
	{"fpcr", A64SysReg_FPCR},
	{"fpsr", A64SysReg_FPSR},
	{"dspsr_el0", A64SysReg_DSPSR_EL0},
	{"dlr_el0", A64SysReg_DLR_EL0},
	{"ifsr32_el2", A64SysReg_IFSR32_EL2},
	{"afsr0_el1", A64SysReg_AFSR0_EL1},
	{"afsr0_el12", A64SysReg_AFSR0_EL12},
	{"afsr0_el2", A64SysReg_AFSR0_EL2},
	{"afsr0_el3", A64SysReg_AFSR0_EL3},
	{"afsr1_el1", A64SysReg_AFSR1_EL1},
	{"afsr1_el12", A64SysReg_AFSR1_EL12},
	{"afsr1_el2", A64SysReg_AFSR1_EL2},
	{"afsr1_el3", A64SysReg_AFSR1_EL3},
	{"esr_el1", A64SysReg_ESR_EL1},
	{"esr_el12", A64SysReg_ESR_EL12},
	{"esr_el2", A64SysReg_ESR_EL2},
	{"esr_el3", A64SysReg_ESR_EL3},
	{"fpexc32_el2", A64SysReg_FPEXC32_EL2},
	{"far_el1", A64SysReg_FAR_EL1},
	{"far_el12", A64SysReg_FAR_EL12},
	{"far_el2", A64SysReg_FAR_EL2},
	{"far_el3", A64SysReg_FAR_EL3},
	{"hpfar_el2", A64SysReg_HPFAR_EL2},
	{"par_el1", A64SysReg_PAR_EL1},
	{"pmcr_el0", A64SysReg_PMCR_EL0},
	{"pmcntenset_el0", A64SysReg_PMCNTENSET_EL0},
	{"pmcntenclr_el0", A64SysReg_PMCNTENCLR_EL0},
	{"pmovsclr_el0", A64SysReg_PMOVSCLR_EL0},
	{"pmselr_el0", A64SysReg_PMSELR_EL0},
	{"pmccntr_el0", A64SysReg_PMCCNTR_EL0},
	{"pmxevtyper_el0", A64SysReg_PMXEVTYPER_EL0},
	{"pmxevcntr_el0", A64SysReg_PMXEVCNTR_EL0},
	{"pmuserenr_el0", A64SysReg_PMUSERENR_EL0},
	{"pmintenset_el1", A64SysReg_PMINTENSET_EL1},
	{"pmintenclr_el1", A64SysReg_PMINTENCLR_EL1},
	{"pmovsset_el0", A64SysReg_PMOVSSET_EL0},
	{"mair_el1", A64SysReg_MAIR_EL1},
	{"mair_el12", A64SysReg_MAIR_EL12},
	{"mair_el2", A64SysReg_MAIR_EL2},
	{"mair_el3", A64SysReg_MAIR_EL3},
	{"amair_el1", A64SysReg_AMAIR_EL1},
	{"amair_el12", A64SysReg_AMAIR_EL12},
	{"amair_el2", A64SysReg_AMAIR_EL2},
	{"amair_el3", A64SysReg_AMAIR_EL3},
	{"vbar_el1", A64SysReg_VBAR_EL1},
	{"vbar_el12", A64SysReg_VBAR_EL12},
	{"vbar_el2", A64SysReg_VBAR_EL2},
	{"vbar_el3", A64SysReg_VBAR_EL3},
	{"rmr_el1", A64SysReg_RMR_EL1},
	{"rmr_el2", A64SysReg_RMR_EL2},
	{"rmr_el3", A64SysReg_RMR_EL3},
	{"contextidr_el1", A64SysReg_CONTEXTIDR_EL1},
	{"contextidr_el12", A64SysReg_CONTEXTIDR_EL12},
	{"contextidr_el2", A64SysReg_CONTEXTIDR_EL2},
	{"tpidr_el0", A64SysReg_TPIDR_EL0},
	{"tpidr_el2", A64SysReg_TPIDR_EL2},
	{"tpidr_el3", A64SysReg_TPIDR_EL3},
	{"tpidrro_el0", A64SysReg_TPIDRRO_EL0},
	{"tpidr_el1", A64SysReg_TPIDR_EL1},
	{"cntfrq_el0", A64SysReg_CNTFRQ_EL0},
	{"cntvoff_el2", A64SysReg_CNTVOFF_EL2},
	{"cntkctl_el1", A64SysReg_CNTKCTL_EL1},
	{"cntkctl_el12", A64SysReg_CNTKCTL_EL12},
	{"cnthctl_el2", A64SysReg_CNTHCTL_EL2},
	{"cntp_tval_el0", A64SysReg_CNTP_TVAL_EL0},
	{"cntp_tval_el02", A64SysReg_CNTP_TVAL_EL02},
	{"cnthp_tval_el2", A64SysReg_CNTHP_TVAL_EL2},
	{"cntps_tval_el1", A64SysReg_CNTPS_TVAL_EL1},
	{"cntp_ctl_el0", A64SysReg_CNTP_CTL_EL0},
	{"cnthp_ctl_el2", A64SysReg_CNTHP_CTL_EL2},
	{"cnthv_ctl_el2", A64SysReg_CNTHVCTL_EL2},
	{"cnthv_cval_el2", A64SysReg_CNTHV_CVAL_EL2},
	{"cnthv_tval_el2", A64SysReg_CNTHV_TVAL_EL2},
	{"cntps_ctl_el1", A64SysReg_CNTPS_CTL_EL1},
	{"cntp_cval_el0", A64SysReg_CNTP_CVAL_EL0},
	{"cntp_cval_el02", A64SysReg_CNTP_CVAL_EL02},
	{"cnthp_cval_el2", A64SysReg_CNTHP_CVAL_EL2},
	{"cntps_cval_el1", A64SysReg_CNTPS_CVAL_EL1},
	{"cntv_tval_el0", A64SysReg_CNTV_TVAL_EL0},
	{"cntv_tval_el02", A64SysReg_CNTV_TVAL_EL02},
	{"cntv_ctl_el0", A64SysReg_CNTV_CTL_EL0},
	{"cntv_ctl_el02", A64SysReg_CNTV_CTL_EL02},
	{"cntv_cval_el0", A64SysReg_CNTV_CVAL_EL0},
	{"cntv_cval_el02", A64SysReg_CNTV_CVAL_EL02},
	{"pmevcntr0_el0", A64SysReg_PMEVCNTR0_EL0},
	{"pmevcntr1_el0", A64SysReg_PMEVCNTR1_EL0},
	{"pmevcntr2_el0", A64SysReg_PMEVCNTR2_EL0},
	{"pmevcntr3_el0", A64SysReg_PMEVCNTR3_EL0},
	{"pmevcntr4_el0", A64SysReg_PMEVCNTR4_EL0},
	{"pmevcntr5_el0", A64SysReg_PMEVCNTR5_EL0},
	{"pmevcntr6_el0", A64SysReg_PMEVCNTR6_EL0},
	{"pmevcntr7_el0", A64SysReg_PMEVCNTR7_EL0},
	{"pmevcntr8_el0", A64SysReg_PMEVCNTR8_EL0},
	{"pmevcntr9_el0", A64SysReg_PMEVCNTR9_EL0},
	{"pmevcntr10_el0", A64SysReg_PMEVCNTR10_EL0},
	{"pmevcntr11_el0", A64SysReg_PMEVCNTR11_EL0},
	{"pmevcntr12_el0", A64SysReg_PMEVCNTR12_EL0},
	{"pmevcntr13_el0", A64SysReg_PMEVCNTR13_EL0},
	{"pmevcntr14_el0", A64SysReg_PMEVCNTR14_EL0},
	{"pmevcntr15_el0", A64SysReg_PMEVCNTR15_EL0},
	{"pmevcntr16_el0", A64SysReg_PMEVCNTR16_EL0},
	{"pmevcntr17_el0", A64SysReg_PMEVCNTR17_EL0},
	{"pmevcntr18_el0", A64SysReg_PMEVCNTR18_EL0},
	{"pmevcntr19_el0", A64SysReg_PMEVCNTR19_EL0},
	{"pmevcntr20_el0", A64SysReg_PMEVCNTR20_EL0},
	{"pmevcntr21_el0", A64SysReg_PMEVCNTR21_EL0},
	{"pmevcntr22_el0", A64SysReg_PMEVCNTR22_EL0},
	{"pmevcntr23_el0", A64SysReg_PMEVCNTR23_EL0},
	{"pmevcntr24_el0", A64SysReg_PMEVCNTR24_EL0},
	{"pmevcntr25_el0", A64SysReg_PMEVCNTR25_EL0},
	{"pmevcntr26_el0", A64SysReg_PMEVCNTR26_EL0},
	{"pmevcntr27_el0", A64SysReg_PMEVCNTR27_EL0},
	{"pmevcntr28_el0", A64SysReg_PMEVCNTR28_EL0},
	{"pmevcntr29_el0", A64SysReg_PMEVCNTR29_EL0},
	{"pmevcntr30_el0", A64SysReg_PMEVCNTR30_EL0},
	{"pmccfiltr_el0", A64SysReg_PMCCFILTR_EL0},
	{"pmevtyper0_el0", A64SysReg_PMEVTYPER0_EL0},
	{"pmevtyper1_el0", A64SysReg_PMEVTYPER1_EL0},
	{"pmevtyper2_el0", A64SysReg_PMEVTYPER2_EL0},
	{"pmevtyper3_el0", A64SysReg_PMEVTYPER3_EL0},
	{"pmevtyper4_el0", A64SysReg_PMEVTYPER4_EL0},
	{"pmevtyper5_el0", A64SysReg_PMEVTYPER5_EL0},
	{"pmevtyper6_el0", A64SysReg_PMEVTYPER6_EL0},
	{"pmevtyper7_el0", A64SysReg_PMEVTYPER7_EL0},
	{"pmevtyper8_el0", A64SysReg_PMEVTYPER8_EL0},
	{"pmevtyper9_el0", A64SysReg_PMEVTYPER9_EL0},
	{"pmevtyper10_el0", A64SysReg_PMEVTYPER10_EL0},
	{"pmevtyper11_el0", A64SysReg_PMEVTYPER11_EL0},
	{"pmevtyper12_el0", A64SysReg_PMEVTYPER12_EL0},
	{"pmevtyper13_el0", A64SysReg_PMEVTYPER13_EL0},
	{"pmevtyper14_el0", A64SysReg_PMEVTYPER14_EL0},
	{"pmevtyper15_el0", A64SysReg_PMEVTYPER15_EL0},
	{"pmevtyper16_el0", A64SysReg_PMEVTYPER16_EL0},
	{"pmevtyper17_el0", A64SysReg_PMEVTYPER17_EL0},
	{"pmevtyper18_el0", A64SysReg_PMEVTYPER18_EL0},
	{"pmevtyper19_el0", A64SysReg_PMEVTYPER19_EL0},
	{"pmevtyper20_el0", A64SysReg_PMEVTYPER20_EL0},
	{"pmevtyper21_el0", A64SysReg_PMEVTYPER21_EL0},
	{"pmevtyper22_el0", A64SysReg_PMEVTYPER22_EL0},
	{"pmevtyper23_el0", A64SysReg_PMEVTYPER23_EL0},
	{"pmevtyper24_el0", A64SysReg_PMEVTYPER24_EL0},
	{"pmevtyper25_el0", A64SysReg_PMEVTYPER25_EL0},
	{"pmevtyper26_el0", A64SysReg_PMEVTYPER26_EL0},
	{"pmevtyper27_el0", A64SysReg_PMEVTYPER27_EL0},
	{"pmevtyper28_el0", A64SysReg_PMEVTYPER28_EL0},
	{"pmevtyper29_el0", A64SysReg_PMEVTYPER29_EL0},
	{"pmevtyper30_el0", A64SysReg_PMEVTYPER30_EL0},
	{"lorc_el1", A64SysReg_LORC_EL1},
	{"lorea_el1", A64SysReg_LOREA_EL1},
	{"lorn_el1", A64SysReg_LORN_EL1},
	{"lorsa_el1", A64SysReg_LORSA_EL1},

	// Trace registers
	{"trcprgctlr", A64SysReg_TRCPRGCTLR},
	{"trcprocselr", A64SysReg_TRCPROCSELR},
	{"trcconfigr", A64SysReg_TRCCONFIGR},
	{"trcauxctlr", A64SysReg_TRCAUXCTLR},
	{"trceventctl0r", A64SysReg_TRCEVENTCTL0R},
	{"trceventctl1r", A64SysReg_TRCEVENTCTL1R},
	{"trcstallctlr", A64SysReg_TRCSTALLCTLR},
	{"trctsctlr", A64SysReg_TRCTSCTLR},
	{"trcsyncpr", A64SysReg_TRCSYNCPR},
	{"trcccctlr", A64SysReg_TRCCCCTLR},
	{"trcbbctlr", A64SysReg_TRCBBCTLR},
	{"trctraceidr", A64SysReg_TRCTRACEIDR},
	{"trcqctlr", A64SysReg_TRCQCTLR},
	{"trcvictlr", A64SysReg_TRCVICTLR},
	{"trcviiectlr", A64SysReg_TRCVIIECTLR},
	{"trcvissctlr", A64SysReg_TRCVISSCTLR},
	{"trcvipcssctlr", A64SysReg_TRCVIPCSSCTLR},
	{"trcvdctlr", A64SysReg_TRCVDCTLR},
	{"trcvdsacctlr", A64SysReg_TRCVDSACCTLR},
	{"trcvdarcctlr", A64SysReg_TRCVDARCCTLR},
	{"trcseqevr0", A64SysReg_TRCSEQEVR0},
	{"trcseqevr1", A64SysReg_TRCSEQEVR1},
	{"trcseqevr2", A64SysReg_TRCSEQEVR2},
	{"trcseqrstevr", A64SysReg_TRCSEQRSTEVR},
	{"trcseqstr", A64SysReg_TRCSEQSTR},
	{"trcextinselr", A64SysReg_TRCEXTINSELR},
	{"trccntrldvr0", A64SysReg_TRCCNTRLDVR0},
	{"trccntrldvr1", A64SysReg_TRCCNTRLDVR1},
	{"trccntrldvr2", A64SysReg_TRCCNTRLDVR2},
	{"trccntrldvr3", A64SysReg_TRCCNTRLDVR3},
	{"trccntctlr0", A64SysReg_TRCCNTCTLR0},
	{"trccntctlr1", A64SysReg_TRCCNTCTLR1},
	{"trccntctlr2", A64SysReg_TRCCNTCTLR2},
	{"trccntctlr3", A64SysReg_TRCCNTCTLR3},
	{"trccntvr0", A64SysReg_TRCCNTVR0},
	{"trccntvr1", A64SysReg_TRCCNTVR1},
	{"trccntvr2", A64SysReg_TRCCNTVR2},
	{"trccntvr3", A64SysReg_TRCCNTVR3},
	{"trcimspec0", A64SysReg_TRCIMSPEC0},
	{"trcimspec1", A64SysReg_TRCIMSPEC1},
	{"trcimspec2", A64SysReg_TRCIMSPEC2},
	{"trcimspec3", A64SysReg_TRCIMSPEC3},
	{"trcimspec4", A64SysReg_TRCIMSPEC4},
	{"trcimspec5", A64SysReg_TRCIMSPEC5},
	{"trcimspec6", A64SysReg_TRCIMSPEC6},
	{"trcimspec7", A64SysReg_TRCIMSPEC7},
	{"trcrsctlr2", A64SysReg_TRCRSCTLR2},
	{"trcrsctlr3", A64SysReg_TRCRSCTLR3},
	{"trcrsctlr4", A64SysReg_TRCRSCTLR4},
	{"trcrsctlr5", A64SysReg_TRCRSCTLR5},
	{"trcrsctlr6", A64SysReg_TRCRSCTLR6},
	{"trcrsctlr7", A64SysReg_TRCRSCTLR7},
	{"trcrsctlr8", A64SysReg_TRCRSCTLR8},
	{"trcrsctlr9", A64SysReg_TRCRSCTLR9},
	{"trcrsctlr10", A64SysReg_TRCRSCTLR10},
	{"trcrsctlr11", A64SysReg_TRCRSCTLR11},
	{"trcrsctlr12", A64SysReg_TRCRSCTLR12},
	{"trcrsctlr13", A64SysReg_TRCRSCTLR13},
	{"trcrsctlr14", A64SysReg_TRCRSCTLR14},
	{"trcrsctlr15", A64SysReg_TRCRSCTLR15},
	{"trcrsctlr16", A64SysReg_TRCRSCTLR16},
	{"trcrsctlr17", A64SysReg_TRCRSCTLR17},
	{"trcrsctlr18", A64SysReg_TRCRSCTLR18},
	{"trcrsctlr19", A64SysReg_TRCRSCTLR19},
	{"trcrsctlr20", A64SysReg_TRCRSCTLR20},
	{"trcrsctlr21", A64SysReg_TRCRSCTLR21},
	{"trcrsctlr22", A64SysReg_TRCRSCTLR22},
	{"trcrsctlr23", A64SysReg_TRCRSCTLR23},
	{"trcrsctlr24", A64SysReg_TRCRSCTLR24},
	{"trcrsctlr25", A64SysReg_TRCRSCTLR25},
	{"trcrsctlr26", A64SysReg_TRCRSCTLR26},
	{"trcrsctlr27", A64SysReg_TRCRSCTLR27},
	{"trcrsctlr28", A64SysReg_TRCRSCTLR28},
	{"trcrsctlr29", A64SysReg_TRCRSCTLR29},
	{"trcrsctlr30", A64SysReg_TRCRSCTLR30},
	{"trcrsctlr31", A64SysReg_TRCRSCTLR31},
	{"trcssccr0", A64SysReg_TRCSSCCR0},
	{"trcssccr1", A64SysReg_TRCSSCCR1},
	{"trcssccr2", A64SysReg_TRCSSCCR2},
	{"trcssccr3", A64SysReg_TRCSSCCR3},
	{"trcssccr4", A64SysReg_TRCSSCCR4},
	{"trcssccr5", A64SysReg_TRCSSCCR5},
	{"trcssccr6", A64SysReg_TRCSSCCR6},
	{"trcssccr7", A64SysReg_TRCSSCCR7},
	{"trcsscsr0", A64SysReg_TRCSSCSR0},
	{"trcsscsr1", A64SysReg_TRCSSCSR1},
	{"trcsscsr2", A64SysReg_TRCSSCSR2},
	{"trcsscsr3", A64SysReg_TRCSSCSR3},
	{"trcsscsr4", A64SysReg_TRCSSCSR4},
	{"trcsscsr5", A64SysReg_TRCSSCSR5},
	{"trcsscsr6", A64SysReg_TRCSSCSR6},
	{"trcsscsr7", A64SysReg_TRCSSCSR7},
	{"trcsspcicr0", A64SysReg_TRCSSPCICR0},
	{"trcsspcicr1", A64SysReg_TRCSSPCICR1},
	{"trcsspcicr2", A64SysReg_TRCSSPCICR2},
	{"trcsspcicr3", A64SysReg_TRCSSPCICR3},
	{"trcsspcicr4", A64SysReg_TRCSSPCICR4},
	{"trcsspcicr5", A64SysReg_TRCSSPCICR5},
	{"trcsspcicr6", A64SysReg_TRCSSPCICR6},
	{"trcsspcicr7", A64SysReg_TRCSSPCICR7},
	{"trcpdcr", A64SysReg_TRCPDCR},
	{"trcacvr0", A64SysReg_TRCACVR0},
	{"trcacvr1", A64SysReg_TRCACVR1},
	{"trcacvr2", A64SysReg_TRCACVR2},
	{"trcacvr3", A64SysReg_TRCACVR3},
	{"trcacvr4", A64SysReg_TRCACVR4},
	{"trcacvr5", A64SysReg_TRCACVR5},
	{"trcacvr6", A64SysReg_TRCACVR6},
	{"trcacvr7", A64SysReg_TRCACVR7},
	{"trcacvr8", A64SysReg_TRCACVR8},
	{"trcacvr9", A64SysReg_TRCACVR9},
	{"trcacvr10", A64SysReg_TRCACVR10},
	{"trcacvr11", A64SysReg_TRCACVR11},
	{"trcacvr12", A64SysReg_TRCACVR12},
	{"trcacvr13", A64SysReg_TRCACVR13},
	{"trcacvr14", A64SysReg_TRCACVR14},
	{"trcacvr15", A64SysReg_TRCACVR15},
	{"trcacatr0", A64SysReg_TRCACATR0},
	{"trcacatr1", A64SysReg_TRCACATR1},
	{"trcacatr2", A64SysReg_TRCACATR2},
	{"trcacatr3", A64SysReg_TRCACATR3},
	{"trcacatr4", A64SysReg_TRCACATR4},
	{"trcacatr5", A64SysReg_TRCACATR5},
	{"trcacatr6", A64SysReg_TRCACATR6},
	{"trcacatr7", A64SysReg_TRCACATR7},
	{"trcacatr8", A64SysReg_TRCACATR8},
	{"trcacatr9", A64SysReg_TRCACATR9},
	{"trcacatr10", A64SysReg_TRCACATR10},
	{"trcacatr11", A64SysReg_TRCACATR11},
	{"trcacatr12", A64SysReg_TRCACATR12},
	{"trcacatr13", A64SysReg_TRCACATR13},
	{"trcacatr14", A64SysReg_TRCACATR14},
	{"trcacatr15", A64SysReg_TRCACATR15},
	{"trcdvcvr0", A64SysReg_TRCDVCVR0},
	{"trcdvcvr1", A64SysReg_TRCDVCVR1},
	{"trcdvcvr2", A64SysReg_TRCDVCVR2},
	{"trcdvcvr3", A64SysReg_TRCDVCVR3},
	{"trcdvcvr4", A64SysReg_TRCDVCVR4},
	{"trcdvcvr5", A64SysReg_TRCDVCVR5},
	{"trcdvcvr6", A64SysReg_TRCDVCVR6},
	{"trcdvcvr7", A64SysReg_TRCDVCVR7},
	{"trcdvcmr0", A64SysReg_TRCDVCMR0},
	{"trcdvcmr1", A64SysReg_TRCDVCMR1},
	{"trcdvcmr2", A64SysReg_TRCDVCMR2},
	{"trcdvcmr3", A64SysReg_TRCDVCMR3},
	{"trcdvcmr4", A64SysReg_TRCDVCMR4},
	{"trcdvcmr5", A64SysReg_TRCDVCMR5},
	{"trcdvcmr6", A64SysReg_TRCDVCMR6},
	{"trcdvcmr7", A64SysReg_TRCDVCMR7},
	{"trccidcvr0", A64SysReg_TRCCIDCVR0},
	{"trccidcvr1", A64SysReg_TRCCIDCVR1},
	{"trccidcvr2", A64SysReg_TRCCIDCVR2},
	{"trccidcvr3", A64SysReg_TRCCIDCVR3},
	{"trccidcvr4", A64SysReg_TRCCIDCVR4},
	{"trccidcvr5", A64SysReg_TRCCIDCVR5},
	{"trccidcvr6", A64SysReg_TRCCIDCVR6},
	{"trccidcvr7", A64SysReg_TRCCIDCVR7},
	{"trcvmidcvr0", A64SysReg_TRCVMIDCVR0},
	{"trcvmidcvr1", A64SysReg_TRCVMIDCVR1},
	{"trcvmidcvr2", A64SysReg_TRCVMIDCVR2},
	{"trcvmidcvr3", A64SysReg_TRCVMIDCVR3},
	{"trcvmidcvr4", A64SysReg_TRCVMIDCVR4},
	{"trcvmidcvr5", A64SysReg_TRCVMIDCVR5},
	{"trcvmidcvr6", A64SysReg_TRCVMIDCVR6},
	{"trcvmidcvr7", A64SysReg_TRCVMIDCVR7},
	{"trccidcctlr0", A64SysReg_TRCCIDCCTLR0},
	{"trccidcctlr1", A64SysReg_TRCCIDCCTLR1},
	{"trcvmidcctlr0", A64SysReg_TRCVMIDCCTLR0},
	{"trcvmidcctlr1", A64SysReg_TRCVMIDCCTLR1},
	{"trcitctrl", A64SysReg_TRCITCTRL},
	{"trcclaimset", A64SysReg_TRCCLAIMSET},
	{"trcclaimclr", A64SysReg_TRCCLAIMCLR},

	// GICv3 registers
	{"icc_bpr1_el1", A64SysReg_ICC_BPR1_EL1},
	{"icc_bpr0_el1", A64SysReg_ICC_BPR0_EL1},
	{"icc_pmr_el1", A64SysReg_ICC_PMR_EL1},
	{"icc_ctlr_el1", A64SysReg_ICC_CTLR_EL1},
	{"icc_ctlr_el3", A64SysReg_ICC_CTLR_EL3},
	{"icc_sre_el1", A64SysReg_ICC_SRE_EL1},
	{"icc_sre_el2", A64SysReg_ICC_SRE_EL2},
	{"icc_sre_el3", A64SysReg_ICC_SRE_EL3},
	{"icc_igrpen0_el1", A64SysReg_ICC_IGRPEN0_EL1},
	{"icc_igrpen1_el1", A64SysReg_ICC_IGRPEN1_EL1},
	{"icc_igrpen1_el3", A64SysReg_ICC_IGRPEN1_EL3},
	{"icc_seien_el1", A64SysReg_ICC_SEIEN_EL1},
	{"icc_ap0r0_el1", A64SysReg_ICC_AP0R0_EL1},
	{"icc_ap0r1_el1", A64SysReg_ICC_AP0R1_EL1},
	{"icc_ap0r2_el1", A64SysReg_ICC_AP0R2_EL1},
	{"icc_ap0r3_el1", A64SysReg_ICC_AP0R3_EL1},
	{"icc_ap1r0_el1", A64SysReg_ICC_AP1R0_EL1},
	{"icc_ap1r1_el1", A64SysReg_ICC_AP1R1_EL1},
	{"icc_ap1r2_el1", A64SysReg_ICC_AP1R2_EL1},
	{"icc_ap1r3_el1", A64SysReg_ICC_AP1R3_EL1},
	{"ich_ap0r0_el2", A64SysReg_ICH_AP0R0_EL2},
	{"ich_ap0r1_el2", A64SysReg_ICH_AP0R1_EL2},
	{"ich_ap0r2_el2", A64SysReg_ICH_AP0R2_EL2},
	{"ich_ap0r3_el2", A64SysReg_ICH_AP0R3_EL2},
	{"ich_ap1r0_el2", A64SysReg_ICH_AP1R0_EL2},
	{"ich_ap1r1_el2", A64SysReg_ICH_AP1R1_EL2},
	{"ich_ap1r2_el2", A64SysReg_ICH_AP1R2_EL2},
	{"ich_ap1r3_el2", A64SysReg_ICH_AP1R3_EL2},
	{"ich_hcr_el2", A64SysReg_ICH_HCR_EL2},
	{"ich_misr_el2", A64SysReg_ICH_MISR_EL2},
	{"ich_vmcr_el2", A64SysReg_ICH_VMCR_EL2},
	{"ich_vseir_el2", A64SysReg_ICH_VSEIR_EL2},
	{"ich_lr0_el2", A64SysReg_ICH_LR0_EL2},
	{"ich_lr1_el2", A64SysReg_ICH_LR1_EL2},
	{"ich_lr2_el2", A64SysReg_ICH_LR2_EL2},
	{"ich_lr3_el2", A64SysReg_ICH_LR3_EL2},
	{"ich_lr4_el2", A64SysReg_ICH_LR4_EL2},
	{"ich_lr5_el2", A64SysReg_ICH_LR5_EL2},
	{"ich_lr6_el2", A64SysReg_ICH_LR6_EL2},
	{"ich_lr7_el2", A64SysReg_ICH_LR7_EL2},
	{"ich_lr8_el2", A64SysReg_ICH_LR8_EL2},
	{"ich_lr9_el2", A64SysReg_ICH_LR9_EL2},
	{"ich_lr10_el2", A64SysReg_ICH_LR10_EL2},
	{"ich_lr11_el2", A64SysReg_ICH_LR11_EL2},
	{"ich_lr12_el2", A64SysReg_ICH_LR12_EL2},
	{"ich_lr13_el2", A64SysReg_ICH_LR13_EL2},
	{"ich_lr14_el2", A64SysReg_ICH_LR14_EL2},
	{"ich_lr15_el2", A64SysReg_ICH_LR15_EL2},

	// Statistical profiling registers
	{"pmblimitr_el1", A64SysReg_PMBLIMITR_EL1},
	{"pmbptr_el1", A64SysReg_PMBPTR_EL1},
	{"pmbsr_el1", A64SysReg_PMBSR_EL1},
	{"pmscr_el1", A64SysReg_PMSCR_EL1},
	{"pmscr_el12", A64SysReg_PMSCR_EL12},
	{"pmscr_el2", A64SysReg_PMSCR_EL2},
	{"pmsicr_el1", A64SysReg_PMSICR_EL1},
	{"pmsirr_el1", A64SysReg_PMSIRR_EL1},
	{"pmsfcr_el1", A64SysReg_PMSFCR_EL1},
	{"pmsevfr_el1", A64SysReg_PMSEVFR_EL1},
	{"pmslatfr_el1", A64SysReg_PMSLATFR_EL1}
};

static const A64NamedImmMapper_Mapping CycloneSysRegPairs[] = {
	{"cpm_ioacc_ctl_el3", A64SysReg_CPM_IOACC_CTL_EL3}
};

// result must be a big enough buffer: 128 bytes is more than enough
void A64SysRegMapper_toString(const A64SysRegMapper *S, uint32_t Bits, char *result)
{
	int dummy;
	uint32_t Op0, Op1, CRn, CRm, Op2;
	char *Op0S, *Op1S, *CRnS, *CRmS, *Op2S;
	unsigned i;

	// First search the registers shared by all
	for (i = 0; i < ARR_SIZE(SysRegPairs); ++i) {
		if (SysRegPairs[i].Value == Bits) {
			strcpy(result, SysRegPairs[i].Name);
			return;
		}
	}

	// Next search for target specific registers
	// if (FeatureBits & AArch64_ProcCyclone) {
	if (true) {
		for (i = 0; i < ARR_SIZE(CycloneSysRegPairs); ++i) {
			if (CycloneSysRegPairs[i].Value == Bits) {
				strcpy(result, CycloneSysRegPairs[i].Name);
				return;
			}
		}
	}

	// Now try the instruction-specific registers (either read-only or
	// write-only).
	for (i = 0; i < S->NumInstPairs; ++i) {
		if (S->InstPairs[i].Value == Bits) {
			strcpy(result, S->InstPairs[i].Name);
			return;
		}
	}

	Op0 = (Bits >> 14) & 0x3;
	Op1 = (Bits >> 11) & 0x7;
	CRn = (Bits >> 7) & 0xf;
	CRm = (Bits >> 3) & 0xf;
	Op2 = Bits & 0x7;

	Op0S = utostr(Op0, false);
	Op1S = utostr(Op1, false);
	CRnS = utostr(CRn, false);
	CRmS = utostr(CRm, false);
	Op2S = utostr(Op2, false);

	//printf("Op1S: %s, CRnS: %s, CRmS: %s, Op2S: %s\n", Op1S, CRnS, CRmS, Op2S);
	dummy = cs_snprintf(result, 128, "s3_%s_c%s_c%s_%s", Op1S, CRnS, CRmS, Op2S);
	(void)dummy;

	cs_mem_free(Op0S);
	cs_mem_free(Op1S);
	cs_mem_free(CRnS);
	cs_mem_free(CRmS);
	cs_mem_free(Op2S);
}

static const A64NamedImmMapper_Mapping TLBIPairs[] = {
	{"ipas2e1is", A64TLBI_IPAS2E1IS},
	{"ipas2le1is", A64TLBI_IPAS2LE1IS},
	{"vmalle1is", A64TLBI_VMALLE1IS},
	{"alle2is", A64TLBI_ALLE2IS},
	{"alle3is", A64TLBI_ALLE3IS},
	{"vae1is", A64TLBI_VAE1IS},
	{"vae2is", A64TLBI_VAE2IS},
	{"vae3is", A64TLBI_VAE3IS},
	{"aside1is", A64TLBI_ASIDE1IS},
	{"vaae1is", A64TLBI_VAAE1IS},
	{"alle1is", A64TLBI_ALLE1IS},
	{"vale1is", A64TLBI_VALE1IS},
	{"vale2is", A64TLBI_VALE2IS},
	{"vale3is", A64TLBI_VALE3IS},
	{"vmalls12e1is", A64TLBI_VMALLS12E1IS},
	{"vaale1is", A64TLBI_VAALE1IS},
	{"ipas2e1", A64TLBI_IPAS2E1},
	{"ipas2le1", A64TLBI_IPAS2LE1},
	{"vmalle1", A64TLBI_VMALLE1},
	{"alle2", A64TLBI_ALLE2},
	{"alle3", A64TLBI_ALLE3},
	{"vae1", A64TLBI_VAE1},
	{"vae2", A64TLBI_VAE2},
	{"vae3", A64TLBI_VAE3},
	{"aside1", A64TLBI_ASIDE1},
	{"vaae1", A64TLBI_VAAE1},
	{"alle1", A64TLBI_ALLE1},
	{"vale1", A64TLBI_VALE1},
	{"vale2", A64TLBI_VALE2},
	{"vale3", A64TLBI_VALE3},
	{"vmalls12e1", A64TLBI_VMALLS12E1},
	{"vaale1", A64TLBI_VAALE1}
};

const A64NamedImmMapper A64TLBI_TLBIMapper = {
	TLBIPairs,
	ARR_SIZE(TLBIPairs),
	0,
};

static const A64NamedImmMapper_Mapping ATPairs[] = {
	{"s1e1r", A64AT_S1E1R},
	{"s1e2r", A64AT_S1E2R},
	{"s1e3r", A64AT_S1E3R},
	{"s1e1w", A64AT_S1E1W},
	{"s1e2w", A64AT_S1E2W},
	{"s1e3w", A64AT_S1E3W},
	{"s1e0r", A64AT_S1E0R},
	{"s1e0w", A64AT_S1E0W},
	{"s12e1r", A64AT_S12E1R},
	{"s12e1w", A64AT_S12E1W},
	{"s12e0r", A64AT_S12E0R},
	{"s12e0w", A64AT_S12E0W}
};

const A64NamedImmMapper A64AT_ATMapper = {
	ATPairs,
	ARR_SIZE(ATPairs),
	0,
};

static const A64NamedImmMapper_Mapping DBarrierPairs[] = {
	{"oshld", A64DB_OSHLD},
	{"oshst", A64DB_OSHST},
	{"osh", A64DB_OSH},
	{"nshld", A64DB_NSHLD},
	{"nshst", A64DB_NSHST},
	{"nsh", A64DB_NSH},
	{"ishld", A64DB_ISHLD},
	{"ishst", A64DB_ISHST},
	{"ish", A64DB_ISH},
	{"ld", A64DB_LD},
	{"st", A64DB_ST},
	{"sy", A64DB_SY}
};

const A64NamedImmMapper A64DB_DBarrierMapper = {
	DBarrierPairs,
	ARR_SIZE(DBarrierPairs),
	16,
};

static const A64NamedImmMapper_Mapping DCPairs[] = {
	{"zva", A64DC_ZVA},
	{"ivac", A64DC_IVAC},
	{"isw", A64DC_ISW},
	{"cvac", A64DC_CVAC},
	{"csw", A64DC_CSW},
	{"cvau", A64DC_CVAU},
	{"civac", A64DC_CIVAC},
	{"cisw", A64DC_CISW}
};

const A64NamedImmMapper A64DC_DCMapper = {
	DCPairs,
	ARR_SIZE(DCPairs),
	0,
};

static const A64NamedImmMapper_Mapping ICPairs[] = {
	{"ialluis",  A64IC_IALLUIS},
	{"iallu", A64IC_IALLU},
	{"ivau", A64IC_IVAU}
};

const A64NamedImmMapper A64IC_ICMapper = {
	ICPairs,
	ARR_SIZE(ICPairs),
	0,
};

static const A64NamedImmMapper_Mapping ISBPairs[] = {
	{"sy",  A64DB_SY},
};

const A64NamedImmMapper A64ISB_ISBMapper = {
	ISBPairs,
	ARR_SIZE(ISBPairs),
	16,
};

static const A64NamedImmMapper_Mapping PRFMPairs[] = {
	{"pldl1keep", A64PRFM_PLDL1KEEP},
	{"pldl1strm", A64PRFM_PLDL1STRM},
	{"pldl2keep", A64PRFM_PLDL2KEEP},
	{"pldl2strm", A64PRFM_PLDL2STRM},
	{"pldl3keep", A64PRFM_PLDL3KEEP},
	{"pldl3strm", A64PRFM_PLDL3STRM},
	{"plil1keep", A64PRFM_PLIL1KEEP},
	{"plil1strm", A64PRFM_PLIL1STRM},
	{"plil2keep", A64PRFM_PLIL2KEEP},
	{"plil2strm", A64PRFM_PLIL2STRM},
	{"plil3keep", A64PRFM_PLIL3KEEP},
	{"plil3strm", A64PRFM_PLIL3STRM},
	{"pstl1keep", A64PRFM_PSTL1KEEP},
	{"pstl1strm", A64PRFM_PSTL1STRM},
	{"pstl2keep", A64PRFM_PSTL2KEEP},
	{"pstl2strm", A64PRFM_PSTL2STRM},
	{"pstl3keep", A64PRFM_PSTL3KEEP},
	{"pstl3strm", A64PRFM_PSTL3STRM}
};

const A64NamedImmMapper A64PRFM_PRFMMapper = {
	PRFMPairs,
	ARR_SIZE(PRFMPairs),
	32,
};

static const A64NamedImmMapper_Mapping PStatePairs[] = {
	{"spsel", A64PState_SPSel},
	{"daifset", A64PState_DAIFSet},
	{"daifclr", A64PState_DAIFClr},
	{"pan", A64PState_PAN},
	{"uao", A64PState_UAO}
};

const A64NamedImmMapper A64PState_PStateMapper = {
	PStatePairs,
	ARR_SIZE(PStatePairs),
	0,
};

static const A64NamedImmMapper_Mapping MRSPairs[] = {
	{"mdccsr_el0", A64SysReg_MDCCSR_EL0},
	{"dbgdtrrx_el0", A64SysReg_DBGDTRRX_EL0},
	{"mdrar_el1", A64SysReg_MDRAR_EL1},
	{"oslsr_el1", A64SysReg_OSLSR_EL1},
	{"dbgauthstatus_el1", A64SysReg_DBGAUTHSTATUS_EL1},
	{"pmceid0_el0", A64SysReg_PMCEID0_EL0},
	{"pmceid1_el0", A64SysReg_PMCEID1_EL0},
	{"midr_el1", A64SysReg_MIDR_EL1},
	{"ccsidr_el1", A64SysReg_CCSIDR_EL1},
	{"clidr_el1", A64SysReg_CLIDR_EL1},
	{"ctr_el0", A64SysReg_CTR_EL0},
	{"mpidr_el1", A64SysReg_MPIDR_EL1},
	{"revidr_el1", A64SysReg_REVIDR_EL1},
	{"aidr_el1", A64SysReg_AIDR_EL1},
	{"dczid_el0", A64SysReg_DCZID_EL0},
	{"id_pfr0_el1", A64SysReg_ID_PFR0_EL1},
	{"id_pfr1_el1", A64SysReg_ID_PFR1_EL1},
	{"id_dfr0_el1", A64SysReg_ID_DFR0_EL1},
	{"id_afr0_el1", A64SysReg_ID_AFR0_EL1},
	{"id_mmfr0_el1", A64SysReg_ID_MMFR0_EL1},
	{"id_mmfr1_el1", A64SysReg_ID_MMFR1_EL1},
	{"id_mmfr2_el1", A64SysReg_ID_MMFR2_EL1},
	{"id_mmfr3_el1", A64SysReg_ID_MMFR3_EL1},
	{"id_mmfr4_el1", A64SysReg_ID_MMFR4_EL1},
	{"id_isar0_el1", A64SysReg_ID_ISAR0_EL1},
	{"id_isar1_el1", A64SysReg_ID_ISAR1_EL1},
	{"id_isar2_el1", A64SysReg_ID_ISAR2_EL1},
	{"id_isar3_el1", A64SysReg_ID_ISAR3_EL1},
	{"id_isar4_el1", A64SysReg_ID_ISAR4_EL1},
	{"id_isar5_el1", A64SysReg_ID_ISAR5_EL1},
	{"id_aa64pfr0_el1", A64SysReg_ID_A64PFR0_EL1},
	{"id_aa64pfr1_el1", A64SysReg_ID_A64PFR1_EL1},
	{"id_aa64dfr0_el1", A64SysReg_ID_A64DFR0_EL1},
	{"id_aa64dfr1_el1", A64SysReg_ID_A64DFR1_EL1},
	{"id_aa64afr0_el1", A64SysReg_ID_A64AFR0_EL1},
	{"id_aa64afr1_el1", A64SysReg_ID_A64AFR1_EL1},
	{"id_aa64isar0_el1", A64SysReg_ID_A64ISAR0_EL1},
	{"id_aa64isar1_el1", A64SysReg_ID_A64ISAR1_EL1},
	{"id_aa64mmfr0_el1", A64SysReg_ID_A64MMFR0_EL1},
	{"id_aa64mmfr1_el1", A64SysReg_ID_A64MMFR1_EL1},
	{"id_aa64mmfr2_el1", A64SysReg_ID_A64MMFR2_EL1},
	{"lorid_el1", A64SysReg_LORID_EL1},
	{"mvfr0_el1", A64SysReg_MVFR0_EL1},
	{"mvfr1_el1", A64SysReg_MVFR1_EL1},
	{"mvfr2_el1", A64SysReg_MVFR2_EL1},
	{"rvbar_el1", A64SysReg_RVBAR_EL1},
	{"rvbar_el2", A64SysReg_RVBAR_EL2},
	{"rvbar_el3", A64SysReg_RVBAR_EL3},
	{"isr_el1", A64SysReg_ISR_EL1},
	{"cntpct_el0", A64SysReg_CNTPCT_EL0},
	{"cntvct_el0", A64SysReg_CNTVCT_EL0},

	// Trace registers
	{"trcstatr", A64SysReg_TRCSTATR},
	{"trcidr8", A64SysReg_TRCIDR8},
	{"trcidr9", A64SysReg_TRCIDR9},
	{"trcidr10", A64SysReg_TRCIDR10},
	{"trcidr11", A64SysReg_TRCIDR11},
	{"trcidr12", A64SysReg_TRCIDR12},
	{"trcidr13", A64SysReg_TRCIDR13},
	{"trcidr0", A64SysReg_TRCIDR0},
	{"trcidr1", A64SysReg_TRCIDR1},
	{"trcidr2", A64SysReg_TRCIDR2},
	{"trcidr3", A64SysReg_TRCIDR3},
	{"trcidr4", A64SysReg_TRCIDR4},
	{"trcidr5", A64SysReg_TRCIDR5},
	{"trcidr6", A64SysReg_TRCIDR6},
	{"trcidr7", A64SysReg_TRCIDR7},
	{"trcoslsr", A64SysReg_TRCOSLSR},
	{"trcpdsr", A64SysReg_TRCPDSR},
	{"trcdevaff0", A64SysReg_TRCDEVAFF0},
	{"trcdevaff1", A64SysReg_TRCDEVAFF1},
	{"trclsr", A64SysReg_TRCLSR},
	{"trcauthstatus", A64SysReg_TRCAUTHSTATUS},
	{"trcdevarch", A64SysReg_TRCDEVARCH},
	{"trcdevid", A64SysReg_TRCDEVID},
	{"trcdevtype", A64SysReg_TRCDEVTYPE},
	{"trcpidr4", A64SysReg_TRCPIDR4},
	{"trcpidr5", A64SysReg_TRCPIDR5},
	{"trcpidr6", A64SysReg_TRCPIDR6},
	{"trcpidr7", A64SysReg_TRCPIDR7},
	{"trcpidr0", A64SysReg_TRCPIDR0},
	{"trcpidr1", A64SysReg_TRCPIDR1},
	{"trcpidr2", A64SysReg_TRCPIDR2},
	{"trcpidr3", A64SysReg_TRCPIDR3},
	{"trccidr0", A64SysReg_TRCCIDR0},
	{"trccidr1", A64SysReg_TRCCIDR1},
	{"trccidr2", A64SysReg_TRCCIDR2},
	{"trccidr3", A64SysReg_TRCCIDR3},

	// GICv3 registers
	{"icc_iar1_el1", A64SysReg_ICC_IAR1_EL1},
	{"icc_iar0_el1", A64SysReg_ICC_IAR0_EL1},
	{"icc_hppir1_el1", A64SysReg_ICC_HPPIR1_EL1},
	{"icc_hppir0_el1", A64SysReg_ICC_HPPIR0_EL1},
	{"icc_rpr_el1", A64SysReg_ICC_RPR_EL1},
	{"ich_vtr_el2", A64SysReg_ICH_VTR_EL2},
	{"ich_eisr_el2", A64SysReg_ICH_EISR_EL2},
	{"ich_elsr_el2", A64SysReg_ICH_ELSR_EL2},

	// Statistical profiling registers
	{"pmsidr_el1", A64SysReg_PMSIDR_EL1},
	{"pmbidr_el1", A64SysReg_PMBIDR_EL1}
};

const A64SysRegMapper AArch64_MRSMapper = {
	NULL,
	MRSPairs,
	ARR_SIZE(MRSPairs),
};

static const A64NamedImmMapper_Mapping MSRPairs[] = {
	{"dbgdtrtx_el0", A64SysReg_DBGDTRTX_EL0},
	{"oslar_el1", A64SysReg_OSLAR_EL1},
	{"pmswinc_el0", A64SysReg_PMSWINC_EL0},

	// Trace registers
	{"trcoslar", A64SysReg_TRCOSLAR},
	{"trclar", A64SysReg_TRCLAR},

	// GICv3 registers
	{"icc_eoir1_el1", A64SysReg_ICC_EOIR1_EL1},
	{"icc_eoir0_el1", A64SysReg_ICC_EOIR0_EL1},
	{"icc_dir_el1", A64SysReg_ICC_DIR_EL1},
	{"icc_sgi1r_el1", A64SysReg_ICC_SGI1R_EL1},
	{"icc_asgi1r_el1", A64SysReg_ICC_ASGI1R_EL1},
	{"icc_sgi0r_el1", A64SysReg_ICC_SGI0R_EL1}
};

const A64SysRegMapper AArch64_MSRMapper = {
	NULL,
	MSRPairs,
	ARR_SIZE(MSRPairs),
};

#endif
