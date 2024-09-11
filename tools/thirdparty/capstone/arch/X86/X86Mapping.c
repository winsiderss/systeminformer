/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_X86

#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#endif

#include <string.h>
#ifndef CAPSTONE_HAS_OSXKERNEL
#include <stdlib.h>
#else
#include "qsort.h"
#endif

#include "X86Mapping.h"
#include "X86DisassemblerDecoder.h"

#include "../../utils.h"


const uint64_t arch_masks[9] = {
	0, 0xff,
	0xffff,
	0,
	0xffffffff,
	0, 0, 0,
	0xffffffffffffffffLL
};

static const x86_reg sib_base_map[] = {
	X86_REG_INVALID,
#define ENTRY(x) X86_REG_##x,
	ALL_SIB_BASES
#undef ENTRY
};

// Fill-ins to make the compiler happy.  These constants are never actually
//   assigned; they are just filler to make an automatically-generated switch
//   statement work.
enum {
	X86_REG_BX_SI = 500,
	X86_REG_BX_DI = 501,
	X86_REG_BP_SI = 502,
	X86_REG_BP_DI = 503,
	X86_REG_sib   = 504,
	X86_REG_sib64 = 505
};

static const x86_reg sib_index_map[] = {
	X86_REG_INVALID,
#define ENTRY(x) X86_REG_##x,
	ALL_EA_BASES
	REGS_XMM
	REGS_YMM
	REGS_ZMM
#undef ENTRY
};

static const x86_reg segment_map[] = {
	X86_REG_INVALID,
	X86_REG_CS,
	X86_REG_SS,
	X86_REG_DS,
	X86_REG_ES,
	X86_REG_FS,
	X86_REG_GS,
};

x86_reg x86_map_sib_base(int r)
{
	return sib_base_map[r];
}

x86_reg x86_map_sib_index(int r)
{
	return sib_index_map[r];
}

x86_reg x86_map_segment(int r)
{
	return segment_map[r];
}

#ifndef CAPSTONE_DIET
static const name_map reg_name_maps[] = {
	{ X86_REG_INVALID, NULL },

	{ X86_REG_AH, "ah" },
	{ X86_REG_AL, "al" },
	{ X86_REG_AX, "ax" },
	{ X86_REG_BH, "bh" },
	{ X86_REG_BL, "bl" },
	{ X86_REG_BP, "bp" },
	{ X86_REG_BPL, "bpl" },
	{ X86_REG_BX, "bx" },
	{ X86_REG_CH, "ch" },
	{ X86_REG_CL, "cl" },
	{ X86_REG_CS, "cs" },
	{ X86_REG_CX, "cx" },
	{ X86_REG_DH, "dh" },
	{ X86_REG_DI, "di" },
	{ X86_REG_DIL, "dil" },
	{ X86_REG_DL, "dl" },
	{ X86_REG_DS, "ds" },
	{ X86_REG_DX, "dx" },
	{ X86_REG_EAX, "eax" },
	{ X86_REG_EBP, "ebp" },
	{ X86_REG_EBX, "ebx" },
	{ X86_REG_ECX, "ecx" },
	{ X86_REG_EDI, "edi" },
	{ X86_REG_EDX, "edx" },
	{ X86_REG_EFLAGS, "flags" },
	{ X86_REG_EIP, "eip" },
	{ X86_REG_EIZ, "eiz" },
	{ X86_REG_ES, "es" },
	{ X86_REG_ESI, "esi" },
	{ X86_REG_ESP, "esp" },
	{ X86_REG_FPSW, "fpsw" },
	{ X86_REG_FS, "fs" },
	{ X86_REG_GS, "gs" },
	{ X86_REG_IP, "ip" },
	{ X86_REG_RAX, "rax" },
	{ X86_REG_RBP, "rbp" },
	{ X86_REG_RBX, "rbx" },
	{ X86_REG_RCX, "rcx" },
	{ X86_REG_RDI, "rdi" },
	{ X86_REG_RDX, "rdx" },
	{ X86_REG_RIP, "rip" },
	{ X86_REG_RIZ, "riz" },
	{ X86_REG_RSI, "rsi" },
	{ X86_REG_RSP, "rsp" },
	{ X86_REG_SI, "si" },
	{ X86_REG_SIL, "sil" },
	{ X86_REG_SP, "sp" },
	{ X86_REG_SPL, "spl" },
	{ X86_REG_SS, "ss" },
	{ X86_REG_CR0, "cr0" },
	{ X86_REG_CR1, "cr1" },
	{ X86_REG_CR2, "cr2" },
	{ X86_REG_CR3, "cr3" },
	{ X86_REG_CR4, "cr4" },
	{ X86_REG_CR5, "cr5" },
	{ X86_REG_CR6, "cr6" },
	{ X86_REG_CR7, "cr7" },
	{ X86_REG_CR8, "cr8" },
	{ X86_REG_CR9, "cr9" },
	{ X86_REG_CR10, "cr10" },
	{ X86_REG_CR11, "cr11" },
	{ X86_REG_CR12, "cr12" },
	{ X86_REG_CR13, "cr13" },
	{ X86_REG_CR14, "cr14" },
	{ X86_REG_CR15, "cr15" },
	{ X86_REG_DR0, "dr0" },
	{ X86_REG_DR1, "dr1" },
	{ X86_REG_DR2, "dr2" },
	{ X86_REG_DR3, "dr3" },
	{ X86_REG_DR4, "dr4" },
	{ X86_REG_DR5, "dr5" },
	{ X86_REG_DR6, "dr6" },
	{ X86_REG_DR7, "dr7" },
	{ X86_REG_DR8, "dr8" },
	{ X86_REG_DR9, "dr9" },
	{ X86_REG_DR10, "dr10" },
	{ X86_REG_DR11, "dr11" },
	{ X86_REG_DR12, "dr12" },
	{ X86_REG_DR13, "dr13" },
	{ X86_REG_DR14, "dr14" },
	{ X86_REG_DR15, "dr15" },
	{ X86_REG_FP0, "fp0" },
	{ X86_REG_FP1, "fp1" },
	{ X86_REG_FP2, "fp2" },
	{ X86_REG_FP3, "fp3" },
	{ X86_REG_FP4, "fp4" },
	{ X86_REG_FP5, "fp5" },
	{ X86_REG_FP6, "fp6" },
	{ X86_REG_FP7, "fp7" },
	{ X86_REG_K0, "k0" },
	{ X86_REG_K1, "k1" },
	{ X86_REG_K2, "k2" },
	{ X86_REG_K3, "k3" },
	{ X86_REG_K4, "k4" },
	{ X86_REG_K5, "k5" },
	{ X86_REG_K6, "k6" },
	{ X86_REG_K7, "k7" },
	{ X86_REG_MM0, "mm0" },
	{ X86_REG_MM1, "mm1" },
	{ X86_REG_MM2, "mm2" },
	{ X86_REG_MM3, "mm3" },
	{ X86_REG_MM4, "mm4" },
	{ X86_REG_MM5, "mm5" },
	{ X86_REG_MM6, "mm6" },
	{ X86_REG_MM7, "mm7" },
	{ X86_REG_R8, "r8" },
	{ X86_REG_R9, "r9" },
	{ X86_REG_R10, "r10" },
	{ X86_REG_R11, "r11" },
	{ X86_REG_R12, "r12" },
	{ X86_REG_R13, "r13" },
	{ X86_REG_R14, "r14" },
	{ X86_REG_R15, "r15" },
	{ X86_REG_ST0, "st(0)" },
	{ X86_REG_ST1, "st(1)" },
	{ X86_REG_ST2, "st(2)" },
	{ X86_REG_ST3, "st(3)" },
	{ X86_REG_ST4, "st(4)" },
	{ X86_REG_ST5, "st(5)" },
	{ X86_REG_ST6, "st(6)" },
	{ X86_REG_ST7, "st(7)" },
	{ X86_REG_XMM0, "xmm0" },
	{ X86_REG_XMM1, "xmm1" },
	{ X86_REG_XMM2, "xmm2" },
	{ X86_REG_XMM3, "xmm3" },
	{ X86_REG_XMM4, "xmm4" },
	{ X86_REG_XMM5, "xmm5" },
	{ X86_REG_XMM6, "xmm6" },
	{ X86_REG_XMM7, "xmm7" },
	{ X86_REG_XMM8, "xmm8" },
	{ X86_REG_XMM9, "xmm9" },
	{ X86_REG_XMM10, "xmm10" },
	{ X86_REG_XMM11, "xmm11" },
	{ X86_REG_XMM12, "xmm12" },
	{ X86_REG_XMM13, "xmm13" },
	{ X86_REG_XMM14, "xmm14" },
	{ X86_REG_XMM15, "xmm15" },
	{ X86_REG_XMM16, "xmm16" },
	{ X86_REG_XMM17, "xmm17" },
	{ X86_REG_XMM18, "xmm18" },
	{ X86_REG_XMM19, "xmm19" },
	{ X86_REG_XMM20, "xmm20" },
	{ X86_REG_XMM21, "xmm21" },
	{ X86_REG_XMM22, "xmm22" },
	{ X86_REG_XMM23, "xmm23" },
	{ X86_REG_XMM24, "xmm24" },
	{ X86_REG_XMM25, "xmm25" },
	{ X86_REG_XMM26, "xmm26" },
	{ X86_REG_XMM27, "xmm27" },
	{ X86_REG_XMM28, "xmm28" },
	{ X86_REG_XMM29, "xmm29" },
	{ X86_REG_XMM30, "xmm30" },
	{ X86_REG_XMM31, "xmm31" },
	{ X86_REG_YMM0, "ymm0" },
	{ X86_REG_YMM1, "ymm1" },
	{ X86_REG_YMM2, "ymm2" },
	{ X86_REG_YMM3, "ymm3" },
	{ X86_REG_YMM4, "ymm4" },
	{ X86_REG_YMM5, "ymm5" },
	{ X86_REG_YMM6, "ymm6" },
	{ X86_REG_YMM7, "ymm7" },
	{ X86_REG_YMM8, "ymm8" },
	{ X86_REG_YMM9, "ymm9" },
	{ X86_REG_YMM10, "ymm10" },
	{ X86_REG_YMM11, "ymm11" },
	{ X86_REG_YMM12, "ymm12" },
	{ X86_REG_YMM13, "ymm13" },
	{ X86_REG_YMM14, "ymm14" },
	{ X86_REG_YMM15, "ymm15" },
	{ X86_REG_YMM16, "ymm16" },
	{ X86_REG_YMM17, "ymm17" },
	{ X86_REG_YMM18, "ymm18" },
	{ X86_REG_YMM19, "ymm19" },
	{ X86_REG_YMM20, "ymm20" },
	{ X86_REG_YMM21, "ymm21" },
	{ X86_REG_YMM22, "ymm22" },
	{ X86_REG_YMM23, "ymm23" },
	{ X86_REG_YMM24, "ymm24" },
	{ X86_REG_YMM25, "ymm25" },
	{ X86_REG_YMM26, "ymm26" },
	{ X86_REG_YMM27, "ymm27" },
	{ X86_REG_YMM28, "ymm28" },
	{ X86_REG_YMM29, "ymm29" },
	{ X86_REG_YMM30, "ymm30" },
	{ X86_REG_YMM31, "ymm31" },
	{ X86_REG_ZMM0, "zmm0" },
	{ X86_REG_ZMM1, "zmm1" },
	{ X86_REG_ZMM2, "zmm2" },
	{ X86_REG_ZMM3, "zmm3" },
	{ X86_REG_ZMM4, "zmm4" },
	{ X86_REG_ZMM5, "zmm5" },
	{ X86_REG_ZMM6, "zmm6" },
	{ X86_REG_ZMM7, "zmm7" },
	{ X86_REG_ZMM8, "zmm8" },
	{ X86_REG_ZMM9, "zmm9" },
	{ X86_REG_ZMM10, "zmm10" },
	{ X86_REG_ZMM11, "zmm11" },
	{ X86_REG_ZMM12, "zmm12" },
	{ X86_REG_ZMM13, "zmm13" },
	{ X86_REG_ZMM14, "zmm14" },
	{ X86_REG_ZMM15, "zmm15" },
	{ X86_REG_ZMM16, "zmm16" },
	{ X86_REG_ZMM17, "zmm17" },
	{ X86_REG_ZMM18, "zmm18" },
	{ X86_REG_ZMM19, "zmm19" },
	{ X86_REG_ZMM20, "zmm20" },
	{ X86_REG_ZMM21, "zmm21" },
	{ X86_REG_ZMM22, "zmm22" },
	{ X86_REG_ZMM23, "zmm23" },
	{ X86_REG_ZMM24, "zmm24" },
	{ X86_REG_ZMM25, "zmm25" },
	{ X86_REG_ZMM26, "zmm26" },
	{ X86_REG_ZMM27, "zmm27" },
	{ X86_REG_ZMM28, "zmm28" },
	{ X86_REG_ZMM29, "zmm29" },
	{ X86_REG_ZMM30, "zmm30" },
	{ X86_REG_ZMM31, "zmm31" },
	{ X86_REG_R8B, "r8b" },
	{ X86_REG_R9B, "r9b" },
	{ X86_REG_R10B, "r10b" },
	{ X86_REG_R11B, "r11b" },
	{ X86_REG_R12B, "r12b" },
	{ X86_REG_R13B, "r13b" },
	{ X86_REG_R14B, "r14b" },
	{ X86_REG_R15B, "r15b" },
	{ X86_REG_R8D, "r8d" },
	{ X86_REG_R9D, "r9d" },
	{ X86_REG_R10D, "r10d" },
	{ X86_REG_R11D, "r11d" },
	{ X86_REG_R12D, "r12d" },
	{ X86_REG_R13D, "r13d" },
	{ X86_REG_R14D, "r14d" },
	{ X86_REG_R15D, "r15d" },
	{ X86_REG_R8W, "r8w" },
	{ X86_REG_R9W, "r9w" },
	{ X86_REG_R10W, "r10w" },
	{ X86_REG_R11W, "r11w" },
	{ X86_REG_R12W, "r12w" },
	{ X86_REG_R13W, "r13w" },
	{ X86_REG_R14W, "r14w" },
	{ X86_REG_R15W, "r15w" },
};
#endif

// register size in non-64bit mode
const uint8_t regsize_map_32 [] = {
	0,	// 	{ X86_REG_INVALID, NULL },
	1,	// { X86_REG_AH, "ah" },
	1,	// { X86_REG_AL, "al" },
	2,	// { X86_REG_AX, "ax" },
	1,	// { X86_REG_BH, "bh" },
	1,	// { X86_REG_BL, "bl" },
	2,	// { X86_REG_BP, "bp" },
	1,	// { X86_REG_BPL, "bpl" },
	2,	// { X86_REG_BX, "bx" },
	1,	// { X86_REG_CH, "ch" },
	1,	// { X86_REG_CL, "cl" },
	2,	// { X86_REG_CS, "cs" },
	2,	// { X86_REG_CX, "cx" },
	1,	// { X86_REG_DH, "dh" },
	2,	// { X86_REG_DI, "di" },
	1,	// { X86_REG_DIL, "dil" },
	1,	// { X86_REG_DL, "dl" },
	2,	// { X86_REG_DS, "ds" },
	2,	// { X86_REG_DX, "dx" },
	4,	// { X86_REG_EAX, "eax" },
	4,	// { X86_REG_EBP, "ebp" },
	4,	// { X86_REG_EBX, "ebx" },
	4,	// { X86_REG_ECX, "ecx" },
	4,	// { X86_REG_EDI, "edi" },
	4,	// { X86_REG_EDX, "edx" },
	4,	// { X86_REG_EFLAGS, "flags" },
	4,	// { X86_REG_EIP, "eip" },
	4,	// { X86_REG_EIZ, "eiz" },
	2,	// { X86_REG_ES, "es" },
	4,	// { X86_REG_ESI, "esi" },
	4,	// { X86_REG_ESP, "esp" },
	10,	// { X86_REG_FPSW, "fpsw" },
	2,	// { X86_REG_FS, "fs" },
	2,	// { X86_REG_GS, "gs" },
	2,	// { X86_REG_IP, "ip" },
	8,	// { X86_REG_RAX, "rax" },
	8,	// { X86_REG_RBP, "rbp" },
	8,	// { X86_REG_RBX, "rbx" },
	8,	// { X86_REG_RCX, "rcx" },
	8,	// { X86_REG_RDI, "rdi" },
	8,	// { X86_REG_RDX, "rdx" },
	8,	// { X86_REG_RIP, "rip" },
	8,	// { X86_REG_RIZ, "riz" },
	8,	// { X86_REG_RSI, "rsi" },
	8,	// { X86_REG_RSP, "rsp" },
	2,	// { X86_REG_SI, "si" },
	1,	// { X86_REG_SIL, "sil" },
	2,	// { X86_REG_SP, "sp" },
	1,	// { X86_REG_SPL, "spl" },
	2,	// { X86_REG_SS, "ss" },
	4,	// { X86_REG_CR0, "cr0" },
	4,	// { X86_REG_CR1, "cr1" },
	4,	// { X86_REG_CR2, "cr2" },
	4,	// { X86_REG_CR3, "cr3" },
	4,	// { X86_REG_CR4, "cr4" },
	8,	// { X86_REG_CR5, "cr5" },
	8,	// { X86_REG_CR6, "cr6" },
	8,	// { X86_REG_CR7, "cr7" },
	8,	// { X86_REG_CR8, "cr8" },
	8,	// { X86_REG_CR9, "cr9" },
	8,	// { X86_REG_CR10, "cr10" },
	8,	// { X86_REG_CR11, "cr11" },
	8,	// { X86_REG_CR12, "cr12" },
	8,	// { X86_REG_CR13, "cr13" },
	8,	// { X86_REG_CR14, "cr14" },
	8,	// { X86_REG_CR15, "cr15" },
	4,	// { X86_REG_DR0, "dr0" },
	4,	// { X86_REG_DR1, "dr1" },
	4,	// { X86_REG_DR2, "dr2" },
	4,	// { X86_REG_DR3, "dr3" },
	4,	// { X86_REG_DR4, "dr4" },
	4,	// { X86_REG_DR5, "dr5" },
	4,	// { X86_REG_DR6, "dr6" },
	4,	// { X86_REG_DR7, "dr7" },
	4,	// { X86_REG_DR8, "dr8" },
	4,	// { X86_REG_DR9, "dr9" },
	4,	// { X86_REG_DR10, "dr10" },
	4,	// { X86_REG_DR11, "dr11" },
	4,	// { X86_REG_DR12, "dr12" },
	4,	// { X86_REG_DR13, "dr13" },
	4,	// { X86_REG_DR14, "dr14" },
	4,	// { X86_REG_DR15, "dr15" },
	10,	// { X86_REG_FP0, "fp0" },
	10,	// { X86_REG_FP1, "fp1" },
	10,	// { X86_REG_FP2, "fp2" },
	10,	// { X86_REG_FP3, "fp3" },
	10,	// { X86_REG_FP4, "fp4" },
	10,	// { X86_REG_FP5, "fp5" },
	10,	// { X86_REG_FP6, "fp6" },
	10,	// { X86_REG_FP7, "fp7" },
	2,	// { X86_REG_K0, "k0" },
	2,	// { X86_REG_K1, "k1" },
	2,	// { X86_REG_K2, "k2" },
	2,	// { X86_REG_K3, "k3" },
	2,	// { X86_REG_K4, "k4" },
	2,	// { X86_REG_K5, "k5" },
	2,	// { X86_REG_K6, "k6" },
	2,	// { X86_REG_K7, "k7" },
	8,	// { X86_REG_MM0, "mm0" },
	8,	// { X86_REG_MM1, "mm1" },
	8,	// { X86_REG_MM2, "mm2" },
	8,	// { X86_REG_MM3, "mm3" },
	8,	// { X86_REG_MM4, "mm4" },
	8,	// { X86_REG_MM5, "mm5" },
	8,	// { X86_REG_MM6, "mm6" },
	8,	// { X86_REG_MM7, "mm7" },
	8,	// { X86_REG_R8, "r8" },
	8,	// { X86_REG_R9, "r9" },
	8,	// { X86_REG_R10, "r10" },
	8,	// { X86_REG_R11, "r11" },
	8,	// { X86_REG_R12, "r12" },
	8,	// { X86_REG_R13, "r13" },
	8,	// { X86_REG_R14, "r14" },
	8,	// { X86_REG_R15, "r15" },
	10,	// { X86_REG_ST0, "st0" },
	10,	// { X86_REG_ST1, "st1" },
	10,	// { X86_REG_ST2, "st2" },
	10,	// { X86_REG_ST3, "st3" },
	10,	// { X86_REG_ST4, "st4" },
	10,	// { X86_REG_ST5, "st5" },
	10,	// { X86_REG_ST6, "st6" },
	10,	// { X86_REG_ST7, "st7" },
	16,	// { X86_REG_XMM0, "xmm0" },
	16,	// { X86_REG_XMM1, "xmm1" },
	16,	// { X86_REG_XMM2, "xmm2" },
	16,	// { X86_REG_XMM3, "xmm3" },
	16,	// { X86_REG_XMM4, "xmm4" },
	16,	// { X86_REG_XMM5, "xmm5" },
	16,	// { X86_REG_XMM6, "xmm6" },
	16,	// { X86_REG_XMM7, "xmm7" },
	16,	// { X86_REG_XMM8, "xmm8" },
	16,	// { X86_REG_XMM9, "xmm9" },
	16,	// { X86_REG_XMM10, "xmm10" },
	16,	// { X86_REG_XMM11, "xmm11" },
	16,	// { X86_REG_XMM12, "xmm12" },
	16,	// { X86_REG_XMM13, "xmm13" },
	16,	// { X86_REG_XMM14, "xmm14" },
	16,	// { X86_REG_XMM15, "xmm15" },
	16,	// { X86_REG_XMM16, "xmm16" },
	16,	// { X86_REG_XMM17, "xmm17" },
	16,	// { X86_REG_XMM18, "xmm18" },
	16,	// { X86_REG_XMM19, "xmm19" },
	16,	// { X86_REG_XMM20, "xmm20" },
	16,	// { X86_REG_XMM21, "xmm21" },
	16,	// { X86_REG_XMM22, "xmm22" },
	16,	// { X86_REG_XMM23, "xmm23" },
	16,	// { X86_REG_XMM24, "xmm24" },
	16,	// { X86_REG_XMM25, "xmm25" },
	16,	// { X86_REG_XMM26, "xmm26" },
	16,	// { X86_REG_XMM27, "xmm27" },
	16,	// { X86_REG_XMM28, "xmm28" },
	16,	// { X86_REG_XMM29, "xmm29" },
	16,	// { X86_REG_XMM30, "xmm30" },
	16,	// { X86_REG_XMM31, "xmm31" },
	32,	// { X86_REG_YMM0, "ymm0" },
	32,	// { X86_REG_YMM1, "ymm1" },
	32,	// { X86_REG_YMM2, "ymm2" },
	32,	// { X86_REG_YMM3, "ymm3" },
	32,	// { X86_REG_YMM4, "ymm4" },
	32,	// { X86_REG_YMM5, "ymm5" },
	32,	// { X86_REG_YMM6, "ymm6" },
	32,	// { X86_REG_YMM7, "ymm7" },
	32,	// { X86_REG_YMM8, "ymm8" },
	32,	// { X86_REG_YMM9, "ymm9" },
	32,	// { X86_REG_YMM10, "ymm10" },
	32,	// { X86_REG_YMM11, "ymm11" },
	32,	// { X86_REG_YMM12, "ymm12" },
	32,	// { X86_REG_YMM13, "ymm13" },
	32,	// { X86_REG_YMM14, "ymm14" },
	32,	// { X86_REG_YMM15, "ymm15" },
	32,	// { X86_REG_YMM16, "ymm16" },
	32,	// { X86_REG_YMM17, "ymm17" },
	32,	// { X86_REG_YMM18, "ymm18" },
	32,	// { X86_REG_YMM19, "ymm19" },
	32,	// { X86_REG_YMM20, "ymm20" },
	32,	// { X86_REG_YMM21, "ymm21" },
	32,	// { X86_REG_YMM22, "ymm22" },
	32,	// { X86_REG_YMM23, "ymm23" },
	32,	// { X86_REG_YMM24, "ymm24" },
	32,	// { X86_REG_YMM25, "ymm25" },
	32,	// { X86_REG_YMM26, "ymm26" },
	32,	// { X86_REG_YMM27, "ymm27" },
	32,	// { X86_REG_YMM28, "ymm28" },
	32,	// { X86_REG_YMM29, "ymm29" },
	32,	// { X86_REG_YMM30, "ymm30" },
	32,	// { X86_REG_YMM31, "ymm31" },
	64,	// { X86_REG_ZMM0, "zmm0" },
	64,	// { X86_REG_ZMM1, "zmm1" },
	64,	// { X86_REG_ZMM2, "zmm2" },
	64,	// { X86_REG_ZMM3, "zmm3" },
	64,	// { X86_REG_ZMM4, "zmm4" },
	64,	// { X86_REG_ZMM5, "zmm5" },
	64,	// { X86_REG_ZMM6, "zmm6" },
	64,	// { X86_REG_ZMM7, "zmm7" },
	64,	// { X86_REG_ZMM8, "zmm8" },
	64,	// { X86_REG_ZMM9, "zmm9" },
	64,	// { X86_REG_ZMM10, "zmm10" },
	64,	// { X86_REG_ZMM11, "zmm11" },
	64,	// { X86_REG_ZMM12, "zmm12" },
	64,	// { X86_REG_ZMM13, "zmm13" },
	64,	// { X86_REG_ZMM14, "zmm14" },
	64,	// { X86_REG_ZMM15, "zmm15" },
	64,	// { X86_REG_ZMM16, "zmm16" },
	64,	// { X86_REG_ZMM17, "zmm17" },
	64,	// { X86_REG_ZMM18, "zmm18" },
	64,	// { X86_REG_ZMM19, "zmm19" },
	64,	// { X86_REG_ZMM20, "zmm20" },
	64,	// { X86_REG_ZMM21, "zmm21" },
	64,	// { X86_REG_ZMM22, "zmm22" },
	64,	// { X86_REG_ZMM23, "zmm23" },
	64,	// { X86_REG_ZMM24, "zmm24" },
	64,	// { X86_REG_ZMM25, "zmm25" },
	64,	// { X86_REG_ZMM26, "zmm26" },
	64,	// { X86_REG_ZMM27, "zmm27" },
	64,	// { X86_REG_ZMM28, "zmm28" },
	64,	// { X86_REG_ZMM29, "zmm29" },
	64,	// { X86_REG_ZMM30, "zmm30" },
	64,	// { X86_REG_ZMM31, "zmm31" },
	1,	// { X86_REG_R8B, "r8b" },
	1,	// { X86_REG_R9B, "r9b" },
	1,	// { X86_REG_R10B, "r10b" },
	1,	// { X86_REG_R11B, "r11b" },
	1,	// { X86_REG_R12B, "r12b" },
	1,	// { X86_REG_R13B, "r13b" },
	1,	// { X86_REG_R14B, "r14b" },
	1,	// { X86_REG_R15B, "r15b" },
	4,	// { X86_REG_R8D, "r8d" },
	4,	// { X86_REG_R9D, "r9d" },
	4,	// { X86_REG_R10D, "r10d" },
	4,	// { X86_REG_R11D, "r11d" },
	4,	// { X86_REG_R12D, "r12d" },
	4,	// { X86_REG_R13D, "r13d" },
	4,	// { X86_REG_R14D, "r14d" },
	4,	// { X86_REG_R15D, "r15d" },
	2,	// { X86_REG_R8W, "r8w" },
	2,	// { X86_REG_R9W, "r9w" },
	2,	// { X86_REG_R10W, "r10w" },
	2,	// { X86_REG_R11W, "r11w" },
	2,	// { X86_REG_R12W, "r12w" },
	2,	// { X86_REG_R13W, "r13w" },
	2,	// { X86_REG_R14W, "r14w" },
	2,	// { X86_REG_R15W, "r15w" },
};

// register size in 64bit mode
const uint8_t regsize_map_64 [] = {
	0,	// 	{ X86_REG_INVALID, NULL },
	1,	// { X86_REG_AH, "ah" },
	1,	// { X86_REG_AL, "al" },
	2,	// { X86_REG_AX, "ax" },
	1,	// { X86_REG_BH, "bh" },
	1,	// { X86_REG_BL, "bl" },
	2,	// { X86_REG_BP, "bp" },
	1,	// { X86_REG_BPL, "bpl" },
	2,	// { X86_REG_BX, "bx" },
	1,	// { X86_REG_CH, "ch" },
	1,	// { X86_REG_CL, "cl" },
	2,	// { X86_REG_CS, "cs" },
	2,	// { X86_REG_CX, "cx" },
	1,	// { X86_REG_DH, "dh" },
	2,	// { X86_REG_DI, "di" },
	1,	// { X86_REG_DIL, "dil" },
	1,	// { X86_REG_DL, "dl" },
	2,	// { X86_REG_DS, "ds" },
	2,	// { X86_REG_DX, "dx" },
	4,	// { X86_REG_EAX, "eax" },
	4,	// { X86_REG_EBP, "ebp" },
	4,	// { X86_REG_EBX, "ebx" },
	4,	// { X86_REG_ECX, "ecx" },
	4,	// { X86_REG_EDI, "edi" },
	4,	// { X86_REG_EDX, "edx" },
	8,	// { X86_REG_EFLAGS, "flags" },
	4,	// { X86_REG_EIP, "eip" },
	4,	// { X86_REG_EIZ, "eiz" },
	2,	// { X86_REG_ES, "es" },
	4,	// { X86_REG_ESI, "esi" },
	4,	// { X86_REG_ESP, "esp" },
	10,	// { X86_REG_FPSW, "fpsw" },
	2,	// { X86_REG_FS, "fs" },
	2,	// { X86_REG_GS, "gs" },
	2,	// { X86_REG_IP, "ip" },
	8,	// { X86_REG_RAX, "rax" },
	8,	// { X86_REG_RBP, "rbp" },
	8,	// { X86_REG_RBX, "rbx" },
	8,	// { X86_REG_RCX, "rcx" },
	8,	// { X86_REG_RDI, "rdi" },
	8,	// { X86_REG_RDX, "rdx" },
	8,	// { X86_REG_RIP, "rip" },
	8,	// { X86_REG_RIZ, "riz" },
	8,	// { X86_REG_RSI, "rsi" },
	8,	// { X86_REG_RSP, "rsp" },
	2,	// { X86_REG_SI, "si" },
	1,	// { X86_REG_SIL, "sil" },
	2,	// { X86_REG_SP, "sp" },
	1,	// { X86_REG_SPL, "spl" },
	2,	// { X86_REG_SS, "ss" },
	8,	// { X86_REG_CR0, "cr0" },
	8,	// { X86_REG_CR1, "cr1" },
	8,	// { X86_REG_CR2, "cr2" },
	8,	// { X86_REG_CR3, "cr3" },
	8,	// { X86_REG_CR4, "cr4" },
	8,	// { X86_REG_CR5, "cr5" },
	8,	// { X86_REG_CR6, "cr6" },
	8,	// { X86_REG_CR7, "cr7" },
	8,	// { X86_REG_CR8, "cr8" },
	8,	// { X86_REG_CR9, "cr9" },
	8,	// { X86_REG_CR10, "cr10" },
	8,	// { X86_REG_CR11, "cr11" },
	8,	// { X86_REG_CR12, "cr12" },
	8,	// { X86_REG_CR13, "cr13" },
	8,	// { X86_REG_CR14, "cr14" },
	8,	// { X86_REG_CR15, "cr15" },
	8,	// { X86_REG_DR0, "dr0" },
	8,	// { X86_REG_DR1, "dr1" },
	8,	// { X86_REG_DR2, "dr2" },
	8,	// { X86_REG_DR3, "dr3" },
	8,	// { X86_REG_DR4, "dr4" },
	8,	// { X86_REG_DR5, "dr5" },
	8,	// { X86_REG_DR6, "dr6" },
	8,	// { X86_REG_DR7, "dr7" },
	8,	// { X86_REG_DR8, "dr8" },
	8,	// { X86_REG_DR9, "dr9" },
	8,	// { X86_REG_DR10, "dr10" },
	8,	// { X86_REG_DR11, "dr11" },
	8,	// { X86_REG_DR12, "dr12" },
	8,	// { X86_REG_DR13, "dr13" },
	8,	// { X86_REG_DR14, "dr14" },
	8,	// { X86_REG_DR15, "dr15" },
	10,	// { X86_REG_FP0, "fp0" },
	10,	// { X86_REG_FP1, "fp1" },
	10,	// { X86_REG_FP2, "fp2" },
	10,	// { X86_REG_FP3, "fp3" },
	10,	// { X86_REG_FP4, "fp4" },
	10,	// { X86_REG_FP5, "fp5" },
	10,	// { X86_REG_FP6, "fp6" },
	10,	// { X86_REG_FP7, "fp7" },
	2,	// { X86_REG_K0, "k0" },
	2,	// { X86_REG_K1, "k1" },
	2,	// { X86_REG_K2, "k2" },
	2,	// { X86_REG_K3, "k3" },
	2,	// { X86_REG_K4, "k4" },
	2,	// { X86_REG_K5, "k5" },
	2,	// { X86_REG_K6, "k6" },
	2,	// { X86_REG_K7, "k7" },
	8,	// { X86_REG_MM0, "mm0" },
	8,	// { X86_REG_MM1, "mm1" },
	8,	// { X86_REG_MM2, "mm2" },
	8,	// { X86_REG_MM3, "mm3" },
	8,	// { X86_REG_MM4, "mm4" },
	8,	// { X86_REG_MM5, "mm5" },
	8,	// { X86_REG_MM6, "mm6" },
	8,	// { X86_REG_MM7, "mm7" },
	8,	// { X86_REG_R8, "r8" },
	8,	// { X86_REG_R9, "r9" },
	8,	// { X86_REG_R10, "r10" },
	8,	// { X86_REG_R11, "r11" },
	8,	// { X86_REG_R12, "r12" },
	8,	// { X86_REG_R13, "r13" },
	8,	// { X86_REG_R14, "r14" },
	8,	// { X86_REG_R15, "r15" },
	10,	// { X86_REG_ST0, "st0" },
	10,	// { X86_REG_ST1, "st1" },
	10,	// { X86_REG_ST2, "st2" },
	10,	// { X86_REG_ST3, "st3" },
	10,	// { X86_REG_ST4, "st4" },
	10,	// { X86_REG_ST5, "st5" },
	10,	// { X86_REG_ST6, "st6" },
	10,	// { X86_REG_ST7, "st7" },
	16,	// { X86_REG_XMM0, "xmm0" },
	16,	// { X86_REG_XMM1, "xmm1" },
	16,	// { X86_REG_XMM2, "xmm2" },
	16,	// { X86_REG_XMM3, "xmm3" },
	16,	// { X86_REG_XMM4, "xmm4" },
	16,	// { X86_REG_XMM5, "xmm5" },
	16,	// { X86_REG_XMM6, "xmm6" },
	16,	// { X86_REG_XMM7, "xmm7" },
	16,	// { X86_REG_XMM8, "xmm8" },
	16,	// { X86_REG_XMM9, "xmm9" },
	16,	// { X86_REG_XMM10, "xmm10" },
	16,	// { X86_REG_XMM11, "xmm11" },
	16,	// { X86_REG_XMM12, "xmm12" },
	16,	// { X86_REG_XMM13, "xmm13" },
	16,	// { X86_REG_XMM14, "xmm14" },
	16,	// { X86_REG_XMM15, "xmm15" },
	16,	// { X86_REG_XMM16, "xmm16" },
	16,	// { X86_REG_XMM17, "xmm17" },
	16,	// { X86_REG_XMM18, "xmm18" },
	16,	// { X86_REG_XMM19, "xmm19" },
	16,	// { X86_REG_XMM20, "xmm20" },
	16,	// { X86_REG_XMM21, "xmm21" },
	16,	// { X86_REG_XMM22, "xmm22" },
	16,	// { X86_REG_XMM23, "xmm23" },
	16,	// { X86_REG_XMM24, "xmm24" },
	16,	// { X86_REG_XMM25, "xmm25" },
	16,	// { X86_REG_XMM26, "xmm26" },
	16,	// { X86_REG_XMM27, "xmm27" },
	16,	// { X86_REG_XMM28, "xmm28" },
	16,	// { X86_REG_XMM29, "xmm29" },
	16,	// { X86_REG_XMM30, "xmm30" },
	16,	// { X86_REG_XMM31, "xmm31" },
	32,	// { X86_REG_YMM0, "ymm0" },
	32,	// { X86_REG_YMM1, "ymm1" },
	32,	// { X86_REG_YMM2, "ymm2" },
	32,	// { X86_REG_YMM3, "ymm3" },
	32,	// { X86_REG_YMM4, "ymm4" },
	32,	// { X86_REG_YMM5, "ymm5" },
	32,	// { X86_REG_YMM6, "ymm6" },
	32,	// { X86_REG_YMM7, "ymm7" },
	32,	// { X86_REG_YMM8, "ymm8" },
	32,	// { X86_REG_YMM9, "ymm9" },
	32,	// { X86_REG_YMM10, "ymm10" },
	32,	// { X86_REG_YMM11, "ymm11" },
	32,	// { X86_REG_YMM12, "ymm12" },
	32,	// { X86_REG_YMM13, "ymm13" },
	32,	// { X86_REG_YMM14, "ymm14" },
	32,	// { X86_REG_YMM15, "ymm15" },
	32,	// { X86_REG_YMM16, "ymm16" },
	32,	// { X86_REG_YMM17, "ymm17" },
	32,	// { X86_REG_YMM18, "ymm18" },
	32,	// { X86_REG_YMM19, "ymm19" },
	32,	// { X86_REG_YMM20, "ymm20" },
	32,	// { X86_REG_YMM21, "ymm21" },
	32,	// { X86_REG_YMM22, "ymm22" },
	32,	// { X86_REG_YMM23, "ymm23" },
	32,	// { X86_REG_YMM24, "ymm24" },
	32,	// { X86_REG_YMM25, "ymm25" },
	32,	// { X86_REG_YMM26, "ymm26" },
	32,	// { X86_REG_YMM27, "ymm27" },
	32,	// { X86_REG_YMM28, "ymm28" },
	32,	// { X86_REG_YMM29, "ymm29" },
	32,	// { X86_REG_YMM30, "ymm30" },
	32,	// { X86_REG_YMM31, "ymm31" },
	64,	// { X86_REG_ZMM0, "zmm0" },
	64,	// { X86_REG_ZMM1, "zmm1" },
	64,	// { X86_REG_ZMM2, "zmm2" },
	64,	// { X86_REG_ZMM3, "zmm3" },
	64,	// { X86_REG_ZMM4, "zmm4" },
	64,	// { X86_REG_ZMM5, "zmm5" },
	64,	// { X86_REG_ZMM6, "zmm6" },
	64,	// { X86_REG_ZMM7, "zmm7" },
	64,	// { X86_REG_ZMM8, "zmm8" },
	64,	// { X86_REG_ZMM9, "zmm9" },
	64,	// { X86_REG_ZMM10, "zmm10" },
	64,	// { X86_REG_ZMM11, "zmm11" },
	64,	// { X86_REG_ZMM12, "zmm12" },
	64,	// { X86_REG_ZMM13, "zmm13" },
	64,	// { X86_REG_ZMM14, "zmm14" },
	64,	// { X86_REG_ZMM15, "zmm15" },
	64,	// { X86_REG_ZMM16, "zmm16" },
	64,	// { X86_REG_ZMM17, "zmm17" },
	64,	// { X86_REG_ZMM18, "zmm18" },
	64,	// { X86_REG_ZMM19, "zmm19" },
	64,	// { X86_REG_ZMM20, "zmm20" },
	64,	// { X86_REG_ZMM21, "zmm21" },
	64,	// { X86_REG_ZMM22, "zmm22" },
	64,	// { X86_REG_ZMM23, "zmm23" },
	64,	// { X86_REG_ZMM24, "zmm24" },
	64,	// { X86_REG_ZMM25, "zmm25" },
	64,	// { X86_REG_ZMM26, "zmm26" },
	64,	// { X86_REG_ZMM27, "zmm27" },
	64,	// { X86_REG_ZMM28, "zmm28" },
	64,	// { X86_REG_ZMM29, "zmm29" },
	64,	// { X86_REG_ZMM30, "zmm30" },
	64,	// { X86_REG_ZMM31, "zmm31" },
	1,	// { X86_REG_R8B, "r8b" },
	1,	// { X86_REG_R9B, "r9b" },
	1,	// { X86_REG_R10B, "r10b" },
	1,	// { X86_REG_R11B, "r11b" },
	1,	// { X86_REG_R12B, "r12b" },
	1,	// { X86_REG_R13B, "r13b" },
	1,	// { X86_REG_R14B, "r14b" },
	1,	// { X86_REG_R15B, "r15b" },
	4,	// { X86_REG_R8D, "r8d" },
	4,	// { X86_REG_R9D, "r9d" },
	4,	// { X86_REG_R10D, "r10d" },
	4,	// { X86_REG_R11D, "r11d" },
	4,	// { X86_REG_R12D, "r12d" },
	4,	// { X86_REG_R13D, "r13d" },
	4,	// { X86_REG_R14D, "r14d" },
	4,	// { X86_REG_R15D, "r15d" },
	2,	// { X86_REG_R8W, "r8w" },
	2,	// { X86_REG_R9W, "r9w" },
	2,	// { X86_REG_R10W, "r10w" },
	2,	// { X86_REG_R11W, "r11w" },
	2,	// { X86_REG_R12W, "r12w" },
	2,	// { X86_REG_R13W, "r13w" },
	2,	// { X86_REG_R14W, "r14w" },
	2,	// { X86_REG_R15W, "r15w" },
};

const char *X86_reg_name(csh handle, unsigned int reg)
{
#ifndef CAPSTONE_DIET
	cs_struct *ud = (cs_struct *)handle;

	if (reg >= ARR_SIZE(reg_name_maps))
		return NULL;

	if (reg == X86_REG_EFLAGS) {
		if (ud->mode & CS_MODE_32)
			return "eflags";
		if (ud->mode & CS_MODE_64)
			return "rflags";
	}

	return reg_name_maps[reg].name;
#else
	return NULL;
#endif
}

#ifndef CAPSTONE_DIET
static const name_map insn_name_maps[] = {
	{ X86_INS_INVALID, NULL },

	{ X86_INS_AAA, "aaa" },
	{ X86_INS_AAD, "aad" },
	{ X86_INS_AAM, "aam" },
	{ X86_INS_AAS, "aas" },
	{ X86_INS_FABS, "fabs" },
	{ X86_INS_ADC, "adc" },
	{ X86_INS_ADCX, "adcx" },
	{ X86_INS_ADD, "add" },
	{ X86_INS_ADDPD, "addpd" },
	{ X86_INS_ADDPS, "addps" },
	{ X86_INS_ADDSD, "addsd" },
	{ X86_INS_ADDSS, "addss" },
	{ X86_INS_ADDSUBPD, "addsubpd" },
	{ X86_INS_ADDSUBPS, "addsubps" },
	{ X86_INS_FADD, "fadd" },
	{ X86_INS_FIADD, "fiadd" },
	{ X86_INS_FADDP, "faddp" },
	{ X86_INS_ADOX, "adox" },
	{ X86_INS_AESDECLAST, "aesdeclast" },
	{ X86_INS_AESDEC, "aesdec" },
	{ X86_INS_AESENCLAST, "aesenclast" },
	{ X86_INS_AESENC, "aesenc" },
	{ X86_INS_AESIMC, "aesimc" },
	{ X86_INS_AESKEYGENASSIST, "aeskeygenassist" },
	{ X86_INS_AND, "and" },
	{ X86_INS_ANDN, "andn" },
	{ X86_INS_ANDNPD, "andnpd" },
	{ X86_INS_ANDNPS, "andnps" },
	{ X86_INS_ANDPD, "andpd" },
	{ X86_INS_ANDPS, "andps" },
	{ X86_INS_ARPL, "arpl" },
	{ X86_INS_BEXTR, "bextr" },
	{ X86_INS_BLCFILL, "blcfill" },
	{ X86_INS_BLCI, "blci" },
	{ X86_INS_BLCIC, "blcic" },
	{ X86_INS_BLCMSK, "blcmsk" },
	{ X86_INS_BLCS, "blcs" },
	{ X86_INS_BLENDPD, "blendpd" },
	{ X86_INS_BLENDPS, "blendps" },
	{ X86_INS_BLENDVPD, "blendvpd" },
	{ X86_INS_BLENDVPS, "blendvps" },
	{ X86_INS_BLSFILL, "blsfill" },
	{ X86_INS_BLSI, "blsi" },
	{ X86_INS_BLSIC, "blsic" },
	{ X86_INS_BLSMSK, "blsmsk" },
	{ X86_INS_BLSR, "blsr" },
	{ X86_INS_BOUND, "bound" },
	{ X86_INS_BSF, "bsf" },
	{ X86_INS_BSR, "bsr" },
	{ X86_INS_BSWAP, "bswap" },
	{ X86_INS_BT, "bt" },
	{ X86_INS_BTC, "btc" },
	{ X86_INS_BTR, "btr" },
	{ X86_INS_BTS, "bts" },
	{ X86_INS_BZHI, "bzhi" },
	{ X86_INS_CALL, "call" },
	{ X86_INS_CBW, "cbw" },
	{ X86_INS_CDQ, "cdq" },
	{ X86_INS_CDQE, "cdqe" },
	{ X86_INS_FCHS, "fchs" },
	{ X86_INS_CLAC, "clac" },
	{ X86_INS_CLC, "clc" },
	{ X86_INS_CLD, "cld" },
	{ X86_INS_CLFLUSH, "clflush" },
	{ X86_INS_CLFLUSHOPT, "clflushopt" },
	{ X86_INS_CLGI, "clgi" },
	{ X86_INS_CLI, "cli" },
	{ X86_INS_CLTS, "clts" },
	{ X86_INS_CLWB, "clwb" },
	{ X86_INS_CMC, "cmc" },
	{ X86_INS_CMOVA, "cmova" },
	{ X86_INS_CMOVAE, "cmovae" },
	{ X86_INS_CMOVB, "cmovb" },
	{ X86_INS_CMOVBE, "cmovbe" },
	{ X86_INS_FCMOVBE, "fcmovbe" },
	{ X86_INS_FCMOVB, "fcmovb" },
	{ X86_INS_CMOVE, "cmove" },
	{ X86_INS_FCMOVE, "fcmove" },
	{ X86_INS_CMOVG, "cmovg" },
	{ X86_INS_CMOVGE, "cmovge" },
	{ X86_INS_CMOVL, "cmovl" },
	{ X86_INS_CMOVLE, "cmovle" },
	{ X86_INS_FCMOVNBE, "fcmovnbe" },
	{ X86_INS_FCMOVNB, "fcmovnb" },
	{ X86_INS_CMOVNE, "cmovne" },
	{ X86_INS_FCMOVNE, "fcmovne" },
	{ X86_INS_CMOVNO, "cmovno" },
	{ X86_INS_CMOVNP, "cmovnp" },
	{ X86_INS_FCMOVNU, "fcmovnu" },
	{ X86_INS_CMOVNS, "cmovns" },
	{ X86_INS_CMOVO, "cmovo" },
	{ X86_INS_CMOVP, "cmovp" },
	{ X86_INS_FCMOVU, "fcmovu" },
	{ X86_INS_CMOVS, "cmovs" },
	{ X86_INS_CMP, "cmp" },
	{ X86_INS_CMPSB, "cmpsb" },
	{ X86_INS_CMPSQ, "cmpsq" },
	{ X86_INS_CMPSW, "cmpsw" },
	{ X86_INS_CMPXCHG16B, "cmpxchg16b" },
	{ X86_INS_CMPXCHG, "cmpxchg" },
	{ X86_INS_CMPXCHG8B, "cmpxchg8b" },
	{ X86_INS_COMISD, "comisd" },
	{ X86_INS_COMISS, "comiss" },
	{ X86_INS_FCOMP, "fcomp" },
	{ X86_INS_FCOMIP, "fcomip" },
	{ X86_INS_FCOMI, "fcomi" },
	{ X86_INS_FCOM, "fcom" },
	{ X86_INS_FCOS, "fcos" },
	{ X86_INS_CPUID, "cpuid" },
	{ X86_INS_CQO, "cqo" },
	{ X86_INS_CRC32, "crc32" },
	{ X86_INS_CVTDQ2PD, "cvtdq2pd" },
	{ X86_INS_CVTDQ2PS, "cvtdq2ps" },
	{ X86_INS_CVTPD2DQ, "cvtpd2dq" },
	{ X86_INS_CVTPD2PS, "cvtpd2ps" },
	{ X86_INS_CVTPS2DQ, "cvtps2dq" },
	{ X86_INS_CVTPS2PD, "cvtps2pd" },
	{ X86_INS_CVTSD2SI, "cvtsd2si" },
	{ X86_INS_CVTSD2SS, "cvtsd2ss" },
	{ X86_INS_CVTSI2SD, "cvtsi2sd" },
	{ X86_INS_CVTSI2SS, "cvtsi2ss" },
	{ X86_INS_CVTSS2SD, "cvtss2sd" },
	{ X86_INS_CVTSS2SI, "cvtss2si" },
	{ X86_INS_CVTTPD2DQ, "cvttpd2dq" },
	{ X86_INS_CVTTPS2DQ, "cvttps2dq" },
	{ X86_INS_CVTTSD2SI, "cvttsd2si" },
	{ X86_INS_CVTTSS2SI, "cvttss2si" },
	{ X86_INS_CWD, "cwd" },
	{ X86_INS_CWDE, "cwde" },
	{ X86_INS_DAA, "daa" },
	{ X86_INS_DAS, "das" },
	{ X86_INS_DATA16, "data16" },
	{ X86_INS_DEC, "dec" },
	{ X86_INS_DIV, "div" },
	{ X86_INS_DIVPD, "divpd" },
	{ X86_INS_DIVPS, "divps" },
	{ X86_INS_FDIVR, "fdivr" },
	{ X86_INS_FIDIVR, "fidivr" },
	{ X86_INS_FDIVRP, "fdivrp" },
	{ X86_INS_DIVSD, "divsd" },
	{ X86_INS_DIVSS, "divss" },
	{ X86_INS_FDIV, "fdiv" },
	{ X86_INS_FIDIV, "fidiv" },
	{ X86_INS_FDIVP, "fdivp" },
	{ X86_INS_DPPD, "dppd" },
	{ X86_INS_DPPS, "dpps" },
	{ X86_INS_RET, "ret" },
	{ X86_INS_ENCLS, "encls" },
	{ X86_INS_ENCLU, "enclu" },
	{ X86_INS_ENTER, "enter" },
	{ X86_INS_EXTRACTPS, "extractps" },
	{ X86_INS_EXTRQ, "extrq" },
	{ X86_INS_F2XM1, "f2xm1" },
	{ X86_INS_LCALL, "lcall" },
	{ X86_INS_LJMP, "ljmp" },
	{ X86_INS_FBLD, "fbld" },
	{ X86_INS_FBSTP, "fbstp" },
	{ X86_INS_FCOMPP, "fcompp" },
	{ X86_INS_FDECSTP, "fdecstp" },
	{ X86_INS_FEMMS, "femms" },
	{ X86_INS_FFREE, "ffree" },
	{ X86_INS_FICOM, "ficom" },
	{ X86_INS_FICOMP, "ficomp" },
	{ X86_INS_FINCSTP, "fincstp" },
	{ X86_INS_FLDCW, "fldcw" },
	{ X86_INS_FLDENV, "fldenv" },
	{ X86_INS_FLDL2E, "fldl2e" },
	{ X86_INS_FLDL2T, "fldl2t" },
	{ X86_INS_FLDLG2, "fldlg2" },
	{ X86_INS_FLDLN2, "fldln2" },
	{ X86_INS_FLDPI, "fldpi" },
	{ X86_INS_FNCLEX, "fnclex" },
	{ X86_INS_FNINIT, "fninit" },
	{ X86_INS_FNOP, "fnop" },
	{ X86_INS_FNSTCW, "fnstcw" },
	{ X86_INS_FNSTSW, "fnstsw" },
	{ X86_INS_FPATAN, "fpatan" },
	{ X86_INS_FPREM, "fprem" },
	{ X86_INS_FPREM1, "fprem1" },
	{ X86_INS_FPTAN, "fptan" },
	{ X86_INS_FFREEP, "ffreep" },
	{ X86_INS_FRNDINT, "frndint" },
	{ X86_INS_FRSTOR, "frstor" },
	{ X86_INS_FNSAVE, "fnsave" },
	{ X86_INS_FSCALE, "fscale" },
	{ X86_INS_FSETPM, "fsetpm" },
	{ X86_INS_FSINCOS, "fsincos" },
	{ X86_INS_FNSTENV, "fnstenv" },
	{ X86_INS_FXAM, "fxam" },
	{ X86_INS_FXRSTOR, "fxrstor" },
	{ X86_INS_FXRSTOR64, "fxrstor64" },
	{ X86_INS_FXSAVE, "fxsave" },
	{ X86_INS_FXSAVE64, "fxsave64" },
	{ X86_INS_FXTRACT, "fxtract" },
	{ X86_INS_FYL2X, "fyl2x" },
	{ X86_INS_FYL2XP1, "fyl2xp1" },
	{ X86_INS_MOVAPD, "movapd" },
	{ X86_INS_MOVAPS, "movaps" },
	{ X86_INS_ORPD, "orpd" },
	{ X86_INS_ORPS, "orps" },
	{ X86_INS_VMOVAPD, "vmovapd" },
	{ X86_INS_VMOVAPS, "vmovaps" },
	{ X86_INS_XORPD, "xorpd" },
	{ X86_INS_XORPS, "xorps" },
	{ X86_INS_GETSEC, "getsec" },
	{ X86_INS_HADDPD, "haddpd" },
	{ X86_INS_HADDPS, "haddps" },
	{ X86_INS_HLT, "hlt" },
	{ X86_INS_HSUBPD, "hsubpd" },
	{ X86_INS_HSUBPS, "hsubps" },
	{ X86_INS_IDIV, "idiv" },
	{ X86_INS_FILD, "fild" },
	{ X86_INS_IMUL, "imul" },
	{ X86_INS_IN, "in" },
	{ X86_INS_INC, "inc" },
	{ X86_INS_INSB, "insb" },
	{ X86_INS_INSERTPS, "insertps" },
	{ X86_INS_INSERTQ, "insertq" },
	{ X86_INS_INSD, "insd" },
	{ X86_INS_INSW, "insw" },
	{ X86_INS_INT, "int" },
	{ X86_INS_INT1, "int1" },
	{ X86_INS_INT3, "int3" },
	{ X86_INS_INTO, "into" },
	{ X86_INS_INVD, "invd" },
	{ X86_INS_INVEPT, "invept" },
	{ X86_INS_INVLPG, "invlpg" },
	{ X86_INS_INVLPGA, "invlpga" },
	{ X86_INS_INVPCID, "invpcid" },
	{ X86_INS_INVVPID, "invvpid" },
	{ X86_INS_IRET, "iret" },
	{ X86_INS_IRETD, "iretd" },
	{ X86_INS_IRETQ, "iretq" },
	{ X86_INS_FISTTP, "fisttp" },
	{ X86_INS_FIST, "fist" },
	{ X86_INS_FISTP, "fistp" },
	{ X86_INS_UCOMISD, "ucomisd" },
	{ X86_INS_UCOMISS, "ucomiss" },
	{ X86_INS_VCOMISD, "vcomisd" },
	{ X86_INS_VCOMISS, "vcomiss" },
	{ X86_INS_VCVTSD2SS, "vcvtsd2ss" },
	{ X86_INS_VCVTSI2SD, "vcvtsi2sd" },
	{ X86_INS_VCVTSI2SS, "vcvtsi2ss" },
	{ X86_INS_VCVTSS2SD, "vcvtss2sd" },
	{ X86_INS_VCVTTSD2SI, "vcvttsd2si" },
	{ X86_INS_VCVTTSD2USI, "vcvttsd2usi" },
	{ X86_INS_VCVTTSS2SI, "vcvttss2si" },
	{ X86_INS_VCVTTSS2USI, "vcvttss2usi" },
	{ X86_INS_VCVTUSI2SD, "vcvtusi2sd" },
	{ X86_INS_VCVTUSI2SS, "vcvtusi2ss" },
	{ X86_INS_VUCOMISD, "vucomisd" },
	{ X86_INS_VUCOMISS, "vucomiss" },
	{ X86_INS_JAE, "jae" },
	{ X86_INS_JA, "ja" },
	{ X86_INS_JBE, "jbe" },
	{ X86_INS_JB, "jb" },
	{ X86_INS_JCXZ, "jcxz" },
	{ X86_INS_JECXZ, "jecxz" },
	{ X86_INS_JE, "je" },
	{ X86_INS_JGE, "jge" },
	{ X86_INS_JG, "jg" },
	{ X86_INS_JLE, "jle" },
	{ X86_INS_JL, "jl" },
	{ X86_INS_JMP, "jmp" },
	{ X86_INS_JNE, "jne" },
	{ X86_INS_JNO, "jno" },
	{ X86_INS_JNP, "jnp" },
	{ X86_INS_JNS, "jns" },
	{ X86_INS_JO, "jo" },
	{ X86_INS_JP, "jp" },
	{ X86_INS_JRCXZ, "jrcxz" },
	{ X86_INS_JS, "js" },
	{ X86_INS_KANDB, "kandb" },
	{ X86_INS_KANDD, "kandd" },
	{ X86_INS_KANDNB, "kandnb" },
	{ X86_INS_KANDND, "kandnd" },
	{ X86_INS_KANDNQ, "kandnq" },
	{ X86_INS_KANDNW, "kandnw" },
	{ X86_INS_KANDQ, "kandq" },
	{ X86_INS_KANDW, "kandw" },
	{ X86_INS_KMOVB, "kmovb" },
	{ X86_INS_KMOVD, "kmovd" },
	{ X86_INS_KMOVQ, "kmovq" },
	{ X86_INS_KMOVW, "kmovw" },
	{ X86_INS_KNOTB, "knotb" },
	{ X86_INS_KNOTD, "knotd" },
	{ X86_INS_KNOTQ, "knotq" },
	{ X86_INS_KNOTW, "knotw" },
	{ X86_INS_KORB, "korb" },
	{ X86_INS_KORD, "kord" },
	{ X86_INS_KORQ, "korq" },
	{ X86_INS_KORTESTB, "kortestb" },
	{ X86_INS_KORTESTD, "kortestd" },
	{ X86_INS_KORTESTQ, "kortestq" },
	{ X86_INS_KORTESTW, "kortestw" },
	{ X86_INS_KORW, "korw" },
	{ X86_INS_KSHIFTLB, "kshiftlb" },
	{ X86_INS_KSHIFTLD, "kshiftld" },
	{ X86_INS_KSHIFTLQ, "kshiftlq" },
	{ X86_INS_KSHIFTLW, "kshiftlw" },
	{ X86_INS_KSHIFTRB, "kshiftrb" },
	{ X86_INS_KSHIFTRD, "kshiftrd" },
	{ X86_INS_KSHIFTRQ, "kshiftrq" },
	{ X86_INS_KSHIFTRW, "kshiftrw" },
	{ X86_INS_KUNPCKBW, "kunpckbw" },
	{ X86_INS_KXNORB, "kxnorb" },
	{ X86_INS_KXNORD, "kxnord" },
	{ X86_INS_KXNORQ, "kxnorq" },
	{ X86_INS_KXNORW, "kxnorw" },
	{ X86_INS_KXORB, "kxorb" },
	{ X86_INS_KXORD, "kxord" },
	{ X86_INS_KXORQ, "kxorq" },
	{ X86_INS_KXORW, "kxorw" },
	{ X86_INS_LAHF, "lahf" },
	{ X86_INS_LAR, "lar" },
	{ X86_INS_LDDQU, "lddqu" },
	{ X86_INS_LDMXCSR, "ldmxcsr" },
	{ X86_INS_LDS, "lds" },
	{ X86_INS_FLDZ, "fldz" },
	{ X86_INS_FLD1, "fld1" },
	{ X86_INS_FLD, "fld" },
	{ X86_INS_LEA, "lea" },
	{ X86_INS_LEAVE, "leave" },
	{ X86_INS_LES, "les" },
	{ X86_INS_LFENCE, "lfence" },
	{ X86_INS_LFS, "lfs" },
	{ X86_INS_LGDT, "lgdt" },
	{ X86_INS_LGS, "lgs" },
	{ X86_INS_LIDT, "lidt" },
	{ X86_INS_LLDT, "lldt" },
	{ X86_INS_LMSW, "lmsw" },
	{ X86_INS_OR, "or" },
	{ X86_INS_SUB, "sub" },
	{ X86_INS_XOR, "xor" },
	{ X86_INS_LODSB, "lodsb" },
	{ X86_INS_LODSD, "lodsd" },
	{ X86_INS_LODSQ, "lodsq" },
	{ X86_INS_LODSW, "lodsw" },
	{ X86_INS_LOOP, "loop" },
	{ X86_INS_LOOPE, "loope" },
	{ X86_INS_LOOPNE, "loopne" },
	{ X86_INS_RETF, "retf" },
	{ X86_INS_RETFQ, "retfq" },
	{ X86_INS_LSL, "lsl" },
	{ X86_INS_LSS, "lss" },
	{ X86_INS_LTR, "ltr" },
	{ X86_INS_XADD, "xadd" },
	{ X86_INS_LZCNT, "lzcnt" },
	{ X86_INS_MASKMOVDQU, "maskmovdqu" },
	{ X86_INS_MAXPD, "maxpd" },
	{ X86_INS_MAXPS, "maxps" },
	{ X86_INS_MAXSD, "maxsd" },
	{ X86_INS_MAXSS, "maxss" },
	{ X86_INS_MFENCE, "mfence" },
	{ X86_INS_MINPD, "minpd" },
	{ X86_INS_MINPS, "minps" },
	{ X86_INS_MINSD, "minsd" },
	{ X86_INS_MINSS, "minss" },
	{ X86_INS_CVTPD2PI, "cvtpd2pi" },
	{ X86_INS_CVTPI2PD, "cvtpi2pd" },
	{ X86_INS_CVTPI2PS, "cvtpi2ps" },
	{ X86_INS_CVTPS2PI, "cvtps2pi" },
	{ X86_INS_CVTTPD2PI, "cvttpd2pi" },
	{ X86_INS_CVTTPS2PI, "cvttps2pi" },
	{ X86_INS_EMMS, "emms" },
	{ X86_INS_MASKMOVQ, "maskmovq" },
	{ X86_INS_MOVD, "movd" },
	{ X86_INS_MOVDQ2Q, "movdq2q" },
	{ X86_INS_MOVNTQ, "movntq" },
	{ X86_INS_MOVQ2DQ, "movq2dq" },
	{ X86_INS_MOVQ, "movq" },
	{ X86_INS_PABSB, "pabsb" },
	{ X86_INS_PABSD, "pabsd" },
	{ X86_INS_PABSW, "pabsw" },
	{ X86_INS_PACKSSDW, "packssdw" },
	{ X86_INS_PACKSSWB, "packsswb" },
	{ X86_INS_PACKUSWB, "packuswb" },
	{ X86_INS_PADDB, "paddb" },
	{ X86_INS_PADDD, "paddd" },
	{ X86_INS_PADDQ, "paddq" },
	{ X86_INS_PADDSB, "paddsb" },
	{ X86_INS_PADDSW, "paddsw" },
	{ X86_INS_PADDUSB, "paddusb" },
	{ X86_INS_PADDUSW, "paddusw" },
	{ X86_INS_PADDW, "paddw" },
	{ X86_INS_PALIGNR, "palignr" },
	{ X86_INS_PANDN, "pandn" },
	{ X86_INS_PAND, "pand" },
	{ X86_INS_PAVGB, "pavgb" },
	{ X86_INS_PAVGW, "pavgw" },
	{ X86_INS_PCMPEQB, "pcmpeqb" },
	{ X86_INS_PCMPEQD, "pcmpeqd" },
	{ X86_INS_PCMPEQW, "pcmpeqw" },
	{ X86_INS_PCMPGTB, "pcmpgtb" },
	{ X86_INS_PCMPGTD, "pcmpgtd" },
	{ X86_INS_PCMPGTW, "pcmpgtw" },
	{ X86_INS_PEXTRW, "pextrw" },
	{ X86_INS_PHADDSW, "phaddsw" },
	{ X86_INS_PHADDW, "phaddw" },
	{ X86_INS_PHADDD, "phaddd" },
	{ X86_INS_PHSUBD, "phsubd" },
	{ X86_INS_PHSUBSW, "phsubsw" },
	{ X86_INS_PHSUBW, "phsubw" },
	{ X86_INS_PINSRW, "pinsrw" },
	{ X86_INS_PMADDUBSW, "pmaddubsw" },
	{ X86_INS_PMADDWD, "pmaddwd" },
	{ X86_INS_PMAXSW, "pmaxsw" },
	{ X86_INS_PMAXUB, "pmaxub" },
	{ X86_INS_PMINSW, "pminsw" },
	{ X86_INS_PMINUB, "pminub" },
	{ X86_INS_PMOVMSKB, "pmovmskb" },
	{ X86_INS_PMULHRSW, "pmulhrsw" },
	{ X86_INS_PMULHUW, "pmulhuw" },
	{ X86_INS_PMULHW, "pmulhw" },
	{ X86_INS_PMULLW, "pmullw" },
	{ X86_INS_PMULUDQ, "pmuludq" },
	{ X86_INS_POR, "por" },
	{ X86_INS_PSADBW, "psadbw" },
	{ X86_INS_PSHUFB, "pshufb" },
	{ X86_INS_PSHUFW, "pshufw" },
	{ X86_INS_PSIGNB, "psignb" },
	{ X86_INS_PSIGND, "psignd" },
	{ X86_INS_PSIGNW, "psignw" },
	{ X86_INS_PSLLD, "pslld" },
	{ X86_INS_PSLLQ, "psllq" },
	{ X86_INS_PSLLW, "psllw" },
	{ X86_INS_PSRAD, "psrad" },
	{ X86_INS_PSRAW, "psraw" },
	{ X86_INS_PSRLD, "psrld" },
	{ X86_INS_PSRLQ, "psrlq" },
	{ X86_INS_PSRLW, "psrlw" },
	{ X86_INS_PSUBB, "psubb" },
	{ X86_INS_PSUBD, "psubd" },
	{ X86_INS_PSUBQ, "psubq" },
	{ X86_INS_PSUBSB, "psubsb" },
	{ X86_INS_PSUBSW, "psubsw" },
	{ X86_INS_PSUBUSB, "psubusb" },
	{ X86_INS_PSUBUSW, "psubusw" },
	{ X86_INS_PSUBW, "psubw" },
	{ X86_INS_PUNPCKHBW, "punpckhbw" },
	{ X86_INS_PUNPCKHDQ, "punpckhdq" },
	{ X86_INS_PUNPCKHWD, "punpckhwd" },
	{ X86_INS_PUNPCKLBW, "punpcklbw" },
	{ X86_INS_PUNPCKLDQ, "punpckldq" },
	{ X86_INS_PUNPCKLWD, "punpcklwd" },
	{ X86_INS_PXOR, "pxor" },
	{ X86_INS_MONITOR, "monitor" },
	{ X86_INS_MONTMUL, "montmul" },
	{ X86_INS_MOV, "mov" },
	{ X86_INS_MOVABS, "movabs" },
	{ X86_INS_MOVBE, "movbe" },
	{ X86_INS_MOVDDUP, "movddup" },
	{ X86_INS_MOVDQA, "movdqa" },
	{ X86_INS_MOVDQU, "movdqu" },
	{ X86_INS_MOVHLPS, "movhlps" },
	{ X86_INS_MOVHPD, "movhpd" },
	{ X86_INS_MOVHPS, "movhps" },
	{ X86_INS_MOVLHPS, "movlhps" },
	{ X86_INS_MOVLPD, "movlpd" },
	{ X86_INS_MOVLPS, "movlps" },
	{ X86_INS_MOVMSKPD, "movmskpd" },
	{ X86_INS_MOVMSKPS, "movmskps" },
	{ X86_INS_MOVNTDQA, "movntdqa" },
	{ X86_INS_MOVNTDQ, "movntdq" },
	{ X86_INS_MOVNTI, "movnti" },
	{ X86_INS_MOVNTPD, "movntpd" },
	{ X86_INS_MOVNTPS, "movntps" },
	{ X86_INS_MOVNTSD, "movntsd" },
	{ X86_INS_MOVNTSS, "movntss" },
	{ X86_INS_MOVSB, "movsb" },
	{ X86_INS_MOVSD, "movsd" },
	{ X86_INS_MOVSHDUP, "movshdup" },
	{ X86_INS_MOVSLDUP, "movsldup" },
	{ X86_INS_MOVSQ, "movsq" },
	{ X86_INS_MOVSS, "movss" },
	{ X86_INS_MOVSW, "movsw" },
	{ X86_INS_MOVSX, "movsx" },
	{ X86_INS_MOVSXD, "movsxd" },
	{ X86_INS_MOVUPD, "movupd" },
	{ X86_INS_MOVUPS, "movups" },
	{ X86_INS_MOVZX, "movzx" },
	{ X86_INS_MPSADBW, "mpsadbw" },
	{ X86_INS_MUL, "mul" },
	{ X86_INS_MULPD, "mulpd" },
	{ X86_INS_MULPS, "mulps" },
	{ X86_INS_MULSD, "mulsd" },
	{ X86_INS_MULSS, "mulss" },
	{ X86_INS_MULX, "mulx" },
	{ X86_INS_FMUL, "fmul" },
	{ X86_INS_FIMUL, "fimul" },
	{ X86_INS_FMULP, "fmulp" },
	{ X86_INS_MWAIT, "mwait" },
	{ X86_INS_NEG, "neg" },
	{ X86_INS_NOP, "nop" },
	{ X86_INS_NOT, "not" },
	{ X86_INS_OUT, "out" },
	{ X86_INS_OUTSB, "outsb" },
	{ X86_INS_OUTSD, "outsd" },
	{ X86_INS_OUTSW, "outsw" },
	{ X86_INS_PACKUSDW, "packusdw" },
	{ X86_INS_PAUSE, "pause" },
	{ X86_INS_PAVGUSB, "pavgusb" },
	{ X86_INS_PBLENDVB, "pblendvb" },
	{ X86_INS_PBLENDW, "pblendw" },
	{ X86_INS_PCLMULQDQ, "pclmulqdq" },
	{ X86_INS_PCMPEQQ, "pcmpeqq" },
	{ X86_INS_PCMPESTRI, "pcmpestri" },
	{ X86_INS_PCMPESTRM, "pcmpestrm" },
	{ X86_INS_PCMPGTQ, "pcmpgtq" },
	{ X86_INS_PCMPISTRI, "pcmpistri" },
	{ X86_INS_PCMPISTRM, "pcmpistrm" },
	{ X86_INS_PCOMMIT, "pcommit" },
	{ X86_INS_PDEP, "pdep" },
	{ X86_INS_PEXT, "pext" },
	{ X86_INS_PEXTRB, "pextrb" },
	{ X86_INS_PEXTRD, "pextrd" },
	{ X86_INS_PEXTRQ, "pextrq" },
	{ X86_INS_PF2ID, "pf2id" },
	{ X86_INS_PF2IW, "pf2iw" },
	{ X86_INS_PFACC, "pfacc" },
	{ X86_INS_PFADD, "pfadd" },
	{ X86_INS_PFCMPEQ, "pfcmpeq" },
	{ X86_INS_PFCMPGE, "pfcmpge" },
	{ X86_INS_PFCMPGT, "pfcmpgt" },
	{ X86_INS_PFMAX, "pfmax" },
	{ X86_INS_PFMIN, "pfmin" },
	{ X86_INS_PFMUL, "pfmul" },
	{ X86_INS_PFNACC, "pfnacc" },
	{ X86_INS_PFPNACC, "pfpnacc" },
	{ X86_INS_PFRCPIT1, "pfrcpit1" },
	{ X86_INS_PFRCPIT2, "pfrcpit2" },
	{ X86_INS_PFRCP, "pfrcp" },
	{ X86_INS_PFRSQIT1, "pfrsqit1" },
	{ X86_INS_PFRSQRT, "pfrsqrt" },
	{ X86_INS_PFSUBR, "pfsubr" },
	{ X86_INS_PFSUB, "pfsub" },
	{ X86_INS_PHMINPOSUW, "phminposuw" },
	{ X86_INS_PI2FD, "pi2fd" },
	{ X86_INS_PI2FW, "pi2fw" },
	{ X86_INS_PINSRB, "pinsrb" },
	{ X86_INS_PINSRD, "pinsrd" },
	{ X86_INS_PINSRQ, "pinsrq" },
	{ X86_INS_PMAXSB, "pmaxsb" },
	{ X86_INS_PMAXSD, "pmaxsd" },
	{ X86_INS_PMAXUD, "pmaxud" },
	{ X86_INS_PMAXUW, "pmaxuw" },
	{ X86_INS_PMINSB, "pminsb" },
	{ X86_INS_PMINSD, "pminsd" },
	{ X86_INS_PMINUD, "pminud" },
	{ X86_INS_PMINUW, "pminuw" },
	{ X86_INS_PMOVSXBD, "pmovsxbd" },
	{ X86_INS_PMOVSXBQ, "pmovsxbq" },
	{ X86_INS_PMOVSXBW, "pmovsxbw" },
	{ X86_INS_PMOVSXDQ, "pmovsxdq" },
	{ X86_INS_PMOVSXWD, "pmovsxwd" },
	{ X86_INS_PMOVSXWQ, "pmovsxwq" },
	{ X86_INS_PMOVZXBD, "pmovzxbd" },
	{ X86_INS_PMOVZXBQ, "pmovzxbq" },
	{ X86_INS_PMOVZXBW, "pmovzxbw" },
	{ X86_INS_PMOVZXDQ, "pmovzxdq" },
	{ X86_INS_PMOVZXWD, "pmovzxwd" },
	{ X86_INS_PMOVZXWQ, "pmovzxwq" },
	{ X86_INS_PMULDQ, "pmuldq" },
	{ X86_INS_PMULHRW, "pmulhrw" },
	{ X86_INS_PMULLD, "pmulld" },
	{ X86_INS_POP, "pop" },
	{ X86_INS_POPAW, "popaw" },
	{ X86_INS_POPAL, "popal" },
	{ X86_INS_POPCNT, "popcnt" },
	{ X86_INS_POPF, "popf" },
	{ X86_INS_POPFD, "popfd" },
	{ X86_INS_POPFQ, "popfq" },
	{ X86_INS_PREFETCH, "prefetch" },
	{ X86_INS_PREFETCHNTA, "prefetchnta" },
	{ X86_INS_PREFETCHT0, "prefetcht0" },
	{ X86_INS_PREFETCHT1, "prefetcht1" },
	{ X86_INS_PREFETCHT2, "prefetcht2" },
	{ X86_INS_PREFETCHW, "prefetchw" },
	{ X86_INS_PSHUFD, "pshufd" },
	{ X86_INS_PSHUFHW, "pshufhw" },
	{ X86_INS_PSHUFLW, "pshuflw" },
	{ X86_INS_PSLLDQ, "pslldq" },
	{ X86_INS_PSRLDQ, "psrldq" },
	{ X86_INS_PSWAPD, "pswapd" },
	{ X86_INS_PTEST, "ptest" },
	{ X86_INS_PUNPCKHQDQ, "punpckhqdq" },
	{ X86_INS_PUNPCKLQDQ, "punpcklqdq" },
	{ X86_INS_PUSH, "push" },
	{ X86_INS_PUSHAW, "pushaw" },
	{ X86_INS_PUSHAL, "pushal" },
	{ X86_INS_PUSHF, "pushf" },
	{ X86_INS_PUSHFD, "pushfd" },
	{ X86_INS_PUSHFQ, "pushfq" },
	{ X86_INS_RCL, "rcl" },
	{ X86_INS_RCPPS, "rcpps" },
	{ X86_INS_RCPSS, "rcpss" },
	{ X86_INS_RCR, "rcr" },
	{ X86_INS_RDFSBASE, "rdfsbase" },
	{ X86_INS_RDGSBASE, "rdgsbase" },
	{ X86_INS_RDMSR, "rdmsr" },
	{ X86_INS_RDPMC, "rdpmc" },
	{ X86_INS_RDRAND, "rdrand" },
	{ X86_INS_RDSEED, "rdseed" },
	{ X86_INS_RDTSC, "rdtsc" },
	{ X86_INS_RDTSCP, "rdtscp" },
	{ X86_INS_ROL, "rol" },
	{ X86_INS_ROR, "ror" },
	{ X86_INS_RORX, "rorx" },
	{ X86_INS_ROUNDPD, "roundpd" },
	{ X86_INS_ROUNDPS, "roundps" },
	{ X86_INS_ROUNDSD, "roundsd" },
	{ X86_INS_ROUNDSS, "roundss" },
	{ X86_INS_RSM, "rsm" },
	{ X86_INS_RSQRTPS, "rsqrtps" },
	{ X86_INS_RSQRTSS, "rsqrtss" },
	{ X86_INS_SAHF, "sahf" },
	{ X86_INS_SAL, "sal" },
	{ X86_INS_SALC, "salc" },
	{ X86_INS_SAR, "sar" },
	{ X86_INS_SARX, "sarx" },
	{ X86_INS_SBB, "sbb" },
	{ X86_INS_SCASB, "scasb" },
	{ X86_INS_SCASD, "scasd" },
	{ X86_INS_SCASQ, "scasq" },
	{ X86_INS_SCASW, "scasw" },
	{ X86_INS_SETAE, "setae" },
	{ X86_INS_SETA, "seta" },
	{ X86_INS_SETBE, "setbe" },
	{ X86_INS_SETB, "setb" },
	{ X86_INS_SETE, "sete" },
	{ X86_INS_SETGE, "setge" },
	{ X86_INS_SETG, "setg" },
	{ X86_INS_SETLE, "setle" },
	{ X86_INS_SETL, "setl" },
	{ X86_INS_SETNE, "setne" },
	{ X86_INS_SETNO, "setno" },
	{ X86_INS_SETNP, "setnp" },
	{ X86_INS_SETNS, "setns" },
	{ X86_INS_SETO, "seto" },
	{ X86_INS_SETP, "setp" },
	{ X86_INS_SETS, "sets" },
	{ X86_INS_SFENCE, "sfence" },
	{ X86_INS_SGDT, "sgdt" },
	{ X86_INS_SHA1MSG1, "sha1msg1" },
	{ X86_INS_SHA1MSG2, "sha1msg2" },
	{ X86_INS_SHA1NEXTE, "sha1nexte" },
	{ X86_INS_SHA1RNDS4, "sha1rnds4" },
	{ X86_INS_SHA256MSG1, "sha256msg1" },
	{ X86_INS_SHA256MSG2, "sha256msg2" },
	{ X86_INS_SHA256RNDS2, "sha256rnds2" },
	{ X86_INS_SHL, "shl" },
	{ X86_INS_SHLD, "shld" },
	{ X86_INS_SHLX, "shlx" },
	{ X86_INS_SHR, "shr" },
	{ X86_INS_SHRD, "shrd" },
	{ X86_INS_SHRX, "shrx" },
	{ X86_INS_SHUFPD, "shufpd" },
	{ X86_INS_SHUFPS, "shufps" },
	{ X86_INS_SIDT, "sidt" },
	{ X86_INS_FSIN, "fsin" },
	{ X86_INS_SKINIT, "skinit" },
	{ X86_INS_SLDT, "sldt" },
	{ X86_INS_SMSW, "smsw" },
	{ X86_INS_SQRTPD, "sqrtpd" },
	{ X86_INS_SQRTPS, "sqrtps" },
	{ X86_INS_SQRTSD, "sqrtsd" },
	{ X86_INS_SQRTSS, "sqrtss" },
	{ X86_INS_FSQRT, "fsqrt" },
	{ X86_INS_STAC, "stac" },
	{ X86_INS_STC, "stc" },
	{ X86_INS_STD, "std" },
	{ X86_INS_STGI, "stgi" },
	{ X86_INS_STI, "sti" },
	{ X86_INS_STMXCSR, "stmxcsr" },
	{ X86_INS_STOSB, "stosb" },
	{ X86_INS_STOSD, "stosd" },
	{ X86_INS_STOSQ, "stosq" },
	{ X86_INS_STOSW, "stosw" },
	{ X86_INS_STR, "str" },
	{ X86_INS_FST, "fst" },
	{ X86_INS_FSTP, "fstp" },
	{ X86_INS_FSTPNCE, "fstpnce" },
	{ X86_INS_FXCH, "fxch" },
	{ X86_INS_SUBPD, "subpd" },
	{ X86_INS_SUBPS, "subps" },
	{ X86_INS_FSUBR, "fsubr" },
	{ X86_INS_FISUBR, "fisubr" },
	{ X86_INS_FSUBRP, "fsubrp" },
	{ X86_INS_SUBSD, "subsd" },
	{ X86_INS_SUBSS, "subss" },
	{ X86_INS_FSUB, "fsub" },
	{ X86_INS_FISUB, "fisub" },
	{ X86_INS_FSUBP, "fsubp" },
	{ X86_INS_SWAPGS, "swapgs" },
	{ X86_INS_SYSCALL, "syscall" },
	{ X86_INS_SYSENTER, "sysenter" },
	{ X86_INS_SYSEXIT, "sysexit" },
	{ X86_INS_SYSRET, "sysret" },
	{ X86_INS_T1MSKC, "t1mskc" },
	{ X86_INS_TEST, "test" },
	{ X86_INS_UD2, "ud2" },
	{ X86_INS_FTST, "ftst" },
	{ X86_INS_TZCNT, "tzcnt" },
	{ X86_INS_TZMSK, "tzmsk" },
	{ X86_INS_FUCOMIP, "fucomip" },
	{ X86_INS_FUCOMI, "fucomi" },
	{ X86_INS_FUCOMPP, "fucompp" },
	{ X86_INS_FUCOMP, "fucomp" },
	{ X86_INS_FUCOM, "fucom" },
	{ X86_INS_UD2B, "ud2b" },
	{ X86_INS_UNPCKHPD, "unpckhpd" },
	{ X86_INS_UNPCKHPS, "unpckhps" },
	{ X86_INS_UNPCKLPD, "unpcklpd" },
	{ X86_INS_UNPCKLPS, "unpcklps" },
	{ X86_INS_VADDPD, "vaddpd" },
	{ X86_INS_VADDPS, "vaddps" },
	{ X86_INS_VADDSD, "vaddsd" },
	{ X86_INS_VADDSS, "vaddss" },
	{ X86_INS_VADDSUBPD, "vaddsubpd" },
	{ X86_INS_VADDSUBPS, "vaddsubps" },
	{ X86_INS_VAESDECLAST, "vaesdeclast" },
	{ X86_INS_VAESDEC, "vaesdec" },
	{ X86_INS_VAESENCLAST, "vaesenclast" },
	{ X86_INS_VAESENC, "vaesenc" },
	{ X86_INS_VAESIMC, "vaesimc" },
	{ X86_INS_VAESKEYGENASSIST, "vaeskeygenassist" },
	{ X86_INS_VALIGND, "valignd" },
	{ X86_INS_VALIGNQ, "valignq" },
	{ X86_INS_VANDNPD, "vandnpd" },
	{ X86_INS_VANDNPS, "vandnps" },
	{ X86_INS_VANDPD, "vandpd" },
	{ X86_INS_VANDPS, "vandps" },
	{ X86_INS_VBLENDMPD, "vblendmpd" },
	{ X86_INS_VBLENDMPS, "vblendmps" },
	{ X86_INS_VBLENDPD, "vblendpd" },
	{ X86_INS_VBLENDPS, "vblendps" },
	{ X86_INS_VBLENDVPD, "vblendvpd" },
	{ X86_INS_VBLENDVPS, "vblendvps" },
	{ X86_INS_VBROADCASTF128, "vbroadcastf128" },
	{ X86_INS_VBROADCASTI32X4, "vbroadcasti32x4" },
	{ X86_INS_VBROADCASTI64X4, "vbroadcasti64x4" },
	{ X86_INS_VBROADCASTSD, "vbroadcastsd" },
	{ X86_INS_VBROADCASTSS, "vbroadcastss" },
	{ X86_INS_VCOMPRESSPD, "vcompresspd" },
	{ X86_INS_VCOMPRESSPS, "vcompressps" },
	{ X86_INS_VCVTDQ2PD, "vcvtdq2pd" },
	{ X86_INS_VCVTDQ2PS, "vcvtdq2ps" },
	{ X86_INS_VCVTPD2DQX, "vcvtpd2dqx" },
	{ X86_INS_VCVTPD2DQ, "vcvtpd2dq" },
	{ X86_INS_VCVTPD2PSX, "vcvtpd2psx" },
	{ X86_INS_VCVTPD2PS, "vcvtpd2ps" },
	{ X86_INS_VCVTPD2UDQ, "vcvtpd2udq" },
	{ X86_INS_VCVTPH2PS, "vcvtph2ps" },
	{ X86_INS_VCVTPS2DQ, "vcvtps2dq" },
	{ X86_INS_VCVTPS2PD, "vcvtps2pd" },
	{ X86_INS_VCVTPS2PH, "vcvtps2ph" },
	{ X86_INS_VCVTPS2UDQ, "vcvtps2udq" },
	{ X86_INS_VCVTSD2SI, "vcvtsd2si" },
	{ X86_INS_VCVTSD2USI, "vcvtsd2usi" },
	{ X86_INS_VCVTSS2SI, "vcvtss2si" },
	{ X86_INS_VCVTSS2USI, "vcvtss2usi" },
	{ X86_INS_VCVTTPD2DQX, "vcvttpd2dqx" },
	{ X86_INS_VCVTTPD2DQ, "vcvttpd2dq" },
	{ X86_INS_VCVTTPD2UDQ, "vcvttpd2udq" },
	{ X86_INS_VCVTTPS2DQ, "vcvttps2dq" },
	{ X86_INS_VCVTTPS2UDQ, "vcvttps2udq" },
	{ X86_INS_VCVTUDQ2PD, "vcvtudq2pd" },
	{ X86_INS_VCVTUDQ2PS, "vcvtudq2ps" },
	{ X86_INS_VDIVPD, "vdivpd" },
	{ X86_INS_VDIVPS, "vdivps" },
	{ X86_INS_VDIVSD, "vdivsd" },
	{ X86_INS_VDIVSS, "vdivss" },
	{ X86_INS_VDPPD, "vdppd" },
	{ X86_INS_VDPPS, "vdpps" },
	{ X86_INS_VERR, "verr" },
	{ X86_INS_VERW, "verw" },
	{ X86_INS_VEXP2PD, "vexp2pd" },
	{ X86_INS_VEXP2PS, "vexp2ps" },
	{ X86_INS_VEXPANDPD, "vexpandpd" },
	{ X86_INS_VEXPANDPS, "vexpandps" },
	{ X86_INS_VEXTRACTF128, "vextractf128" },
	{ X86_INS_VEXTRACTF32X4, "vextractf32x4" },
	{ X86_INS_VEXTRACTF64X4, "vextractf64x4" },
	{ X86_INS_VEXTRACTI128, "vextracti128" },
	{ X86_INS_VEXTRACTI32X4, "vextracti32x4" },
	{ X86_INS_VEXTRACTI64X4, "vextracti64x4" },
	{ X86_INS_VEXTRACTPS, "vextractps" },
	{ X86_INS_VFMADD132PD, "vfmadd132pd" },
	{ X86_INS_VFMADD132PS, "vfmadd132ps" },
	{ X86_INS_VFMADDPD, "vfmaddpd" },
	{ X86_INS_VFMADD213PD, "vfmadd213pd" },
	{ X86_INS_VFMADD231PD, "vfmadd231pd" },
	{ X86_INS_VFMADDPS, "vfmaddps" },
	{ X86_INS_VFMADD213PS, "vfmadd213ps" },
	{ X86_INS_VFMADD231PS, "vfmadd231ps" },
	{ X86_INS_VFMADDSD, "vfmaddsd" },
	{ X86_INS_VFMADD213SD, "vfmadd213sd" },
	{ X86_INS_VFMADD132SD, "vfmadd132sd" },
	{ X86_INS_VFMADD231SD, "vfmadd231sd" },
	{ X86_INS_VFMADDSS, "vfmaddss" },
	{ X86_INS_VFMADD213SS, "vfmadd213ss" },
	{ X86_INS_VFMADD132SS, "vfmadd132ss" },
	{ X86_INS_VFMADD231SS, "vfmadd231ss" },
	{ X86_INS_VFMADDSUB132PD, "vfmaddsub132pd" },
	{ X86_INS_VFMADDSUB132PS, "vfmaddsub132ps" },
	{ X86_INS_VFMADDSUBPD, "vfmaddsubpd" },
	{ X86_INS_VFMADDSUB213PD, "vfmaddsub213pd" },
	{ X86_INS_VFMADDSUB231PD, "vfmaddsub231pd" },
	{ X86_INS_VFMADDSUBPS, "vfmaddsubps" },
	{ X86_INS_VFMADDSUB213PS, "vfmaddsub213ps" },
	{ X86_INS_VFMADDSUB231PS, "vfmaddsub231ps" },
	{ X86_INS_VFMSUB132PD, "vfmsub132pd" },
	{ X86_INS_VFMSUB132PS, "vfmsub132ps" },
	{ X86_INS_VFMSUBADD132PD, "vfmsubadd132pd" },
	{ X86_INS_VFMSUBADD132PS, "vfmsubadd132ps" },
	{ X86_INS_VFMSUBADDPD, "vfmsubaddpd" },
	{ X86_INS_VFMSUBADD213PD, "vfmsubadd213pd" },
	{ X86_INS_VFMSUBADD231PD, "vfmsubadd231pd" },
	{ X86_INS_VFMSUBADDPS, "vfmsubaddps" },
	{ X86_INS_VFMSUBADD213PS, "vfmsubadd213ps" },
	{ X86_INS_VFMSUBADD231PS, "vfmsubadd231ps" },
	{ X86_INS_VFMSUBPD, "vfmsubpd" },
	{ X86_INS_VFMSUB213PD, "vfmsub213pd" },
	{ X86_INS_VFMSUB231PD, "vfmsub231pd" },
	{ X86_INS_VFMSUBPS, "vfmsubps" },
	{ X86_INS_VFMSUB213PS, "vfmsub213ps" },
	{ X86_INS_VFMSUB231PS, "vfmsub231ps" },
	{ X86_INS_VFMSUBSD, "vfmsubsd" },
	{ X86_INS_VFMSUB213SD, "vfmsub213sd" },
	{ X86_INS_VFMSUB132SD, "vfmsub132sd" },
	{ X86_INS_VFMSUB231SD, "vfmsub231sd" },
	{ X86_INS_VFMSUBSS, "vfmsubss" },
	{ X86_INS_VFMSUB213SS, "vfmsub213ss" },
	{ X86_INS_VFMSUB132SS, "vfmsub132ss" },
	{ X86_INS_VFMSUB231SS, "vfmsub231ss" },
	{ X86_INS_VFNMADD132PD, "vfnmadd132pd" },
	{ X86_INS_VFNMADD132PS, "vfnmadd132ps" },
	{ X86_INS_VFNMADDPD, "vfnmaddpd" },
	{ X86_INS_VFNMADD213PD, "vfnmadd213pd" },
	{ X86_INS_VFNMADD231PD, "vfnmadd231pd" },
	{ X86_INS_VFNMADDPS, "vfnmaddps" },
	{ X86_INS_VFNMADD213PS, "vfnmadd213ps" },
	{ X86_INS_VFNMADD231PS, "vfnmadd231ps" },
	{ X86_INS_VFNMADDSD, "vfnmaddsd" },
	{ X86_INS_VFNMADD213SD, "vfnmadd213sd" },
	{ X86_INS_VFNMADD132SD, "vfnmadd132sd" },
	{ X86_INS_VFNMADD231SD, "vfnmadd231sd" },
	{ X86_INS_VFNMADDSS, "vfnmaddss" },
	{ X86_INS_VFNMADD213SS, "vfnmadd213ss" },
	{ X86_INS_VFNMADD132SS, "vfnmadd132ss" },
	{ X86_INS_VFNMADD231SS, "vfnmadd231ss" },
	{ X86_INS_VFNMSUB132PD, "vfnmsub132pd" },
	{ X86_INS_VFNMSUB132PS, "vfnmsub132ps" },
	{ X86_INS_VFNMSUBPD, "vfnmsubpd" },
	{ X86_INS_VFNMSUB213PD, "vfnmsub213pd" },
	{ X86_INS_VFNMSUB231PD, "vfnmsub231pd" },
	{ X86_INS_VFNMSUBPS, "vfnmsubps" },
	{ X86_INS_VFNMSUB213PS, "vfnmsub213ps" },
	{ X86_INS_VFNMSUB231PS, "vfnmsub231ps" },
	{ X86_INS_VFNMSUBSD, "vfnmsubsd" },
	{ X86_INS_VFNMSUB213SD, "vfnmsub213sd" },
	{ X86_INS_VFNMSUB132SD, "vfnmsub132sd" },
	{ X86_INS_VFNMSUB231SD, "vfnmsub231sd" },
	{ X86_INS_VFNMSUBSS, "vfnmsubss" },
	{ X86_INS_VFNMSUB213SS, "vfnmsub213ss" },
	{ X86_INS_VFNMSUB132SS, "vfnmsub132ss" },
	{ X86_INS_VFNMSUB231SS, "vfnmsub231ss" },
	{ X86_INS_VFRCZPD, "vfrczpd" },
	{ X86_INS_VFRCZPS, "vfrczps" },
	{ X86_INS_VFRCZSD, "vfrczsd" },
	{ X86_INS_VFRCZSS, "vfrczss" },
	{ X86_INS_VORPD, "vorpd" },
	{ X86_INS_VORPS, "vorps" },
	{ X86_INS_VXORPD, "vxorpd" },
	{ X86_INS_VXORPS, "vxorps" },
	{ X86_INS_VGATHERDPD, "vgatherdpd" },
	{ X86_INS_VGATHERDPS, "vgatherdps" },
	{ X86_INS_VGATHERPF0DPD, "vgatherpf0dpd" },
	{ X86_INS_VGATHERPF0DPS, "vgatherpf0dps" },
	{ X86_INS_VGATHERPF0QPD, "vgatherpf0qpd" },
	{ X86_INS_VGATHERPF0QPS, "vgatherpf0qps" },
	{ X86_INS_VGATHERPF1DPD, "vgatherpf1dpd" },
	{ X86_INS_VGATHERPF1DPS, "vgatherpf1dps" },
	{ X86_INS_VGATHERPF1QPD, "vgatherpf1qpd" },
	{ X86_INS_VGATHERPF1QPS, "vgatherpf1qps" },
	{ X86_INS_VGATHERQPD, "vgatherqpd" },
	{ X86_INS_VGATHERQPS, "vgatherqps" },
	{ X86_INS_VHADDPD, "vhaddpd" },
	{ X86_INS_VHADDPS, "vhaddps" },
	{ X86_INS_VHSUBPD, "vhsubpd" },
	{ X86_INS_VHSUBPS, "vhsubps" },
	{ X86_INS_VINSERTF128, "vinsertf128" },
	{ X86_INS_VINSERTF32X4, "vinsertf32x4" },
	{ X86_INS_VINSERTF32X8, "vinsertf32x8" },
	{ X86_INS_VINSERTF64X2, "vinsertf64x2" },
	{ X86_INS_VINSERTF64X4, "vinsertf64x4" },
	{ X86_INS_VINSERTI128, "vinserti128" },
	{ X86_INS_VINSERTI32X4, "vinserti32x4" },
	{ X86_INS_VINSERTI32X8, "vinserti32x8" },
	{ X86_INS_VINSERTI64X2, "vinserti64x2" },
	{ X86_INS_VINSERTI64X4, "vinserti64x4" },
	{ X86_INS_VINSERTPS, "vinsertps" },
	{ X86_INS_VLDDQU, "vlddqu" },
	{ X86_INS_VLDMXCSR, "vldmxcsr" },
	{ X86_INS_VMASKMOVDQU, "vmaskmovdqu" },
	{ X86_INS_VMASKMOVPD, "vmaskmovpd" },
	{ X86_INS_VMASKMOVPS, "vmaskmovps" },
	{ X86_INS_VMAXPD, "vmaxpd" },
	{ X86_INS_VMAXPS, "vmaxps" },
	{ X86_INS_VMAXSD, "vmaxsd" },
	{ X86_INS_VMAXSS, "vmaxss" },
	{ X86_INS_VMCALL, "vmcall" },
	{ X86_INS_VMCLEAR, "vmclear" },
	{ X86_INS_VMFUNC, "vmfunc" },
	{ X86_INS_VMINPD, "vminpd" },
	{ X86_INS_VMINPS, "vminps" },
	{ X86_INS_VMINSD, "vminsd" },
	{ X86_INS_VMINSS, "vminss" },
	{ X86_INS_VMLAUNCH, "vmlaunch" },
	{ X86_INS_VMLOAD, "vmload" },
	{ X86_INS_VMMCALL, "vmmcall" },
	{ X86_INS_VMOVQ, "vmovq" },
	{ X86_INS_VMOVDDUP, "vmovddup" },
	{ X86_INS_VMOVD, "vmovd" },
	{ X86_INS_VMOVDQA32, "vmovdqa32" },
	{ X86_INS_VMOVDQA64, "vmovdqa64" },
	{ X86_INS_VMOVDQA, "vmovdqa" },
	{ X86_INS_VMOVDQU16, "vmovdqu16" },
	{ X86_INS_VMOVDQU32, "vmovdqu32" },
	{ X86_INS_VMOVDQU64, "vmovdqu64" },
	{ X86_INS_VMOVDQU8, "vmovdqu8" },
	{ X86_INS_VMOVDQU, "vmovdqu" },
	{ X86_INS_VMOVHLPS, "vmovhlps" },
	{ X86_INS_VMOVHPD, "vmovhpd" },
	{ X86_INS_VMOVHPS, "vmovhps" },
	{ X86_INS_VMOVLHPS, "vmovlhps" },
	{ X86_INS_VMOVLPD, "vmovlpd" },
	{ X86_INS_VMOVLPS, "vmovlps" },
	{ X86_INS_VMOVMSKPD, "vmovmskpd" },
	{ X86_INS_VMOVMSKPS, "vmovmskps" },
	{ X86_INS_VMOVNTDQA, "vmovntdqa" },
	{ X86_INS_VMOVNTDQ, "vmovntdq" },
	{ X86_INS_VMOVNTPD, "vmovntpd" },
	{ X86_INS_VMOVNTPS, "vmovntps" },
	{ X86_INS_VMOVSD, "vmovsd" },
	{ X86_INS_VMOVSHDUP, "vmovshdup" },
	{ X86_INS_VMOVSLDUP, "vmovsldup" },
	{ X86_INS_VMOVSS, "vmovss" },
	{ X86_INS_VMOVUPD, "vmovupd" },
	{ X86_INS_VMOVUPS, "vmovups" },
	{ X86_INS_VMPSADBW, "vmpsadbw" },
	{ X86_INS_VMPTRLD, "vmptrld" },
	{ X86_INS_VMPTRST, "vmptrst" },
	{ X86_INS_VMREAD, "vmread" },
	{ X86_INS_VMRESUME, "vmresume" },
	{ X86_INS_VMRUN, "vmrun" },
	{ X86_INS_VMSAVE, "vmsave" },
	{ X86_INS_VMULPD, "vmulpd" },
	{ X86_INS_VMULPS, "vmulps" },
	{ X86_INS_VMULSD, "vmulsd" },
	{ X86_INS_VMULSS, "vmulss" },
	{ X86_INS_VMWRITE, "vmwrite" },
	{ X86_INS_VMXOFF, "vmxoff" },
	{ X86_INS_VMXON, "vmxon" },
	{ X86_INS_VPABSB, "vpabsb" },
	{ X86_INS_VPABSD, "vpabsd" },
	{ X86_INS_VPABSQ, "vpabsq" },
	{ X86_INS_VPABSW, "vpabsw" },
	{ X86_INS_VPACKSSDW, "vpackssdw" },
	{ X86_INS_VPACKSSWB, "vpacksswb" },
	{ X86_INS_VPACKUSDW, "vpackusdw" },
	{ X86_INS_VPACKUSWB, "vpackuswb" },
	{ X86_INS_VPADDB, "vpaddb" },
	{ X86_INS_VPADDD, "vpaddd" },
	{ X86_INS_VPADDQ, "vpaddq" },
	{ X86_INS_VPADDSB, "vpaddsb" },
	{ X86_INS_VPADDSW, "vpaddsw" },
	{ X86_INS_VPADDUSB, "vpaddusb" },
	{ X86_INS_VPADDUSW, "vpaddusw" },
	{ X86_INS_VPADDW, "vpaddw" },
	{ X86_INS_VPALIGNR, "vpalignr" },
	{ X86_INS_VPANDD, "vpandd" },
	{ X86_INS_VPANDND, "vpandnd" },
	{ X86_INS_VPANDNQ, "vpandnq" },
	{ X86_INS_VPANDN, "vpandn" },
	{ X86_INS_VPANDQ, "vpandq" },
	{ X86_INS_VPAND, "vpand" },
	{ X86_INS_VPAVGB, "vpavgb" },
	{ X86_INS_VPAVGW, "vpavgw" },
	{ X86_INS_VPBLENDD, "vpblendd" },
	{ X86_INS_VPBLENDMB, "vpblendmb" },
	{ X86_INS_VPBLENDMD, "vpblendmd" },
	{ X86_INS_VPBLENDMQ, "vpblendmq" },
	{ X86_INS_VPBLENDMW, "vpblendmw" },
	{ X86_INS_VPBLENDVB, "vpblendvb" },
	{ X86_INS_VPBLENDW, "vpblendw" },
	{ X86_INS_VPBROADCASTB, "vpbroadcastb" },
	{ X86_INS_VPBROADCASTD, "vpbroadcastd" },
	{ X86_INS_VPBROADCASTMB2Q, "vpbroadcastmb2q" },
	{ X86_INS_VPBROADCASTMW2D, "vpbroadcastmw2d" },
	{ X86_INS_VPBROADCASTQ, "vpbroadcastq" },
	{ X86_INS_VPBROADCASTW, "vpbroadcastw" },
	{ X86_INS_VPCLMULQDQ, "vpclmulqdq" },
	{ X86_INS_VPCMOV, "vpcmov" },
	{ X86_INS_VPCMPB, "vpcmpb" },
	{ X86_INS_VPCMPD, "vpcmpd" },
	{ X86_INS_VPCMPEQB, "vpcmpeqb" },
	{ X86_INS_VPCMPEQD, "vpcmpeqd" },
	{ X86_INS_VPCMPEQQ, "vpcmpeqq" },
	{ X86_INS_VPCMPEQW, "vpcmpeqw" },
	{ X86_INS_VPCMPESTRI, "vpcmpestri" },
	{ X86_INS_VPCMPESTRM, "vpcmpestrm" },
	{ X86_INS_VPCMPGTB, "vpcmpgtb" },
	{ X86_INS_VPCMPGTD, "vpcmpgtd" },
	{ X86_INS_VPCMPGTQ, "vpcmpgtq" },
	{ X86_INS_VPCMPGTW, "vpcmpgtw" },
	{ X86_INS_VPCMPISTRI, "vpcmpistri" },
	{ X86_INS_VPCMPISTRM, "vpcmpistrm" },
	{ X86_INS_VPCMPQ, "vpcmpq" },
	{ X86_INS_VPCMPUB, "vpcmpub" },
	{ X86_INS_VPCMPUD, "vpcmpud" },
	{ X86_INS_VPCMPUQ, "vpcmpuq" },
	{ X86_INS_VPCMPUW, "vpcmpuw" },
	{ X86_INS_VPCMPW, "vpcmpw" },
	{ X86_INS_VPCOMB, "vpcomb" },
	{ X86_INS_VPCOMD, "vpcomd" },
	{ X86_INS_VPCOMPRESSD, "vpcompressd" },
	{ X86_INS_VPCOMPRESSQ, "vpcompressq" },
	{ X86_INS_VPCOMQ, "vpcomq" },
	{ X86_INS_VPCOMUB, "vpcomub" },
	{ X86_INS_VPCOMUD, "vpcomud" },
	{ X86_INS_VPCOMUQ, "vpcomuq" },
	{ X86_INS_VPCOMUW, "vpcomuw" },
	{ X86_INS_VPCOMW, "vpcomw" },
	{ X86_INS_VPCONFLICTD, "vpconflictd" },
	{ X86_INS_VPCONFLICTQ, "vpconflictq" },
	{ X86_INS_VPERM2F128, "vperm2f128" },
	{ X86_INS_VPERM2I128, "vperm2i128" },
	{ X86_INS_VPERMD, "vpermd" },
	{ X86_INS_VPERMI2D, "vpermi2d" },
	{ X86_INS_VPERMI2PD, "vpermi2pd" },
	{ X86_INS_VPERMI2PS, "vpermi2ps" },
	{ X86_INS_VPERMI2Q, "vpermi2q" },
	{ X86_INS_VPERMIL2PD, "vpermil2pd" },
	{ X86_INS_VPERMIL2PS, "vpermil2ps" },
	{ X86_INS_VPERMILPD, "vpermilpd" },
	{ X86_INS_VPERMILPS, "vpermilps" },
	{ X86_INS_VPERMPD, "vpermpd" },
	{ X86_INS_VPERMPS, "vpermps" },
	{ X86_INS_VPERMQ, "vpermq" },
	{ X86_INS_VPERMT2D, "vpermt2d" },
	{ X86_INS_VPERMT2PD, "vpermt2pd" },
	{ X86_INS_VPERMT2PS, "vpermt2ps" },
	{ X86_INS_VPERMT2Q, "vpermt2q" },
	{ X86_INS_VPEXPANDD, "vpexpandd" },
	{ X86_INS_VPEXPANDQ, "vpexpandq" },
	{ X86_INS_VPEXTRB, "vpextrb" },
	{ X86_INS_VPEXTRD, "vpextrd" },
	{ X86_INS_VPEXTRQ, "vpextrq" },
	{ X86_INS_VPEXTRW, "vpextrw" },
	{ X86_INS_VPGATHERDD, "vpgatherdd" },
	{ X86_INS_VPGATHERDQ, "vpgatherdq" },
	{ X86_INS_VPGATHERQD, "vpgatherqd" },
	{ X86_INS_VPGATHERQQ, "vpgatherqq" },
	{ X86_INS_VPHADDBD, "vphaddbd" },
	{ X86_INS_VPHADDBQ, "vphaddbq" },
	{ X86_INS_VPHADDBW, "vphaddbw" },
	{ X86_INS_VPHADDDQ, "vphadddq" },
	{ X86_INS_VPHADDD, "vphaddd" },
	{ X86_INS_VPHADDSW, "vphaddsw" },
	{ X86_INS_VPHADDUBD, "vphaddubd" },
	{ X86_INS_VPHADDUBQ, "vphaddubq" },
	{ X86_INS_VPHADDUBW, "vphaddubw" },
	{ X86_INS_VPHADDUDQ, "vphaddudq" },
	{ X86_INS_VPHADDUWD, "vphadduwd" },
	{ X86_INS_VPHADDUWQ, "vphadduwq" },
	{ X86_INS_VPHADDWD, "vphaddwd" },
	{ X86_INS_VPHADDWQ, "vphaddwq" },
	{ X86_INS_VPHADDW, "vphaddw" },
	{ X86_INS_VPHMINPOSUW, "vphminposuw" },
	{ X86_INS_VPHSUBBW, "vphsubbw" },
	{ X86_INS_VPHSUBDQ, "vphsubdq" },
	{ X86_INS_VPHSUBD, "vphsubd" },
	{ X86_INS_VPHSUBSW, "vphsubsw" },
	{ X86_INS_VPHSUBWD, "vphsubwd" },
	{ X86_INS_VPHSUBW, "vphsubw" },
	{ X86_INS_VPINSRB, "vpinsrb" },
	{ X86_INS_VPINSRD, "vpinsrd" },
	{ X86_INS_VPINSRQ, "vpinsrq" },
	{ X86_INS_VPINSRW, "vpinsrw" },
	{ X86_INS_VPLZCNTD, "vplzcntd" },
	{ X86_INS_VPLZCNTQ, "vplzcntq" },
	{ X86_INS_VPMACSDD, "vpmacsdd" },
	{ X86_INS_VPMACSDQH, "vpmacsdqh" },
	{ X86_INS_VPMACSDQL, "vpmacsdql" },
	{ X86_INS_VPMACSSDD, "vpmacssdd" },
	{ X86_INS_VPMACSSDQH, "vpmacssdqh" },
	{ X86_INS_VPMACSSDQL, "vpmacssdql" },
	{ X86_INS_VPMACSSWD, "vpmacsswd" },
	{ X86_INS_VPMACSSWW, "vpmacssww" },
	{ X86_INS_VPMACSWD, "vpmacswd" },
	{ X86_INS_VPMACSWW, "vpmacsww" },
	{ X86_INS_VPMADCSSWD, "vpmadcsswd" },
	{ X86_INS_VPMADCSWD, "vpmadcswd" },
	{ X86_INS_VPMADDUBSW, "vpmaddubsw" },
	{ X86_INS_VPMADDWD, "vpmaddwd" },
	{ X86_INS_VPMASKMOVD, "vpmaskmovd" },
	{ X86_INS_VPMASKMOVQ, "vpmaskmovq" },
	{ X86_INS_VPMAXSB, "vpmaxsb" },
	{ X86_INS_VPMAXSD, "vpmaxsd" },
	{ X86_INS_VPMAXSQ, "vpmaxsq" },
	{ X86_INS_VPMAXSW, "vpmaxsw" },
	{ X86_INS_VPMAXUB, "vpmaxub" },
	{ X86_INS_VPMAXUD, "vpmaxud" },
	{ X86_INS_VPMAXUQ, "vpmaxuq" },
	{ X86_INS_VPMAXUW, "vpmaxuw" },
	{ X86_INS_VPMINSB, "vpminsb" },
	{ X86_INS_VPMINSD, "vpminsd" },
	{ X86_INS_VPMINSQ, "vpminsq" },
	{ X86_INS_VPMINSW, "vpminsw" },
	{ X86_INS_VPMINUB, "vpminub" },
	{ X86_INS_VPMINUD, "vpminud" },
	{ X86_INS_VPMINUQ, "vpminuq" },
	{ X86_INS_VPMINUW, "vpminuw" },
	{ X86_INS_VPMOVDB, "vpmovdb" },
	{ X86_INS_VPMOVDW, "vpmovdw" },
	{ X86_INS_VPMOVM2B, "vpmovm2b" },
	{ X86_INS_VPMOVM2D, "vpmovm2d" },
	{ X86_INS_VPMOVM2Q, "vpmovm2q" },
	{ X86_INS_VPMOVM2W, "vpmovm2w" },
	{ X86_INS_VPMOVMSKB, "vpmovmskb" },
	{ X86_INS_VPMOVQB, "vpmovqb" },
	{ X86_INS_VPMOVQD, "vpmovqd" },
	{ X86_INS_VPMOVQW, "vpmovqw" },
	{ X86_INS_VPMOVSDB, "vpmovsdb" },
	{ X86_INS_VPMOVSDW, "vpmovsdw" },
	{ X86_INS_VPMOVSQB, "vpmovsqb" },
	{ X86_INS_VPMOVSQD, "vpmovsqd" },
	{ X86_INS_VPMOVSQW, "vpmovsqw" },
	{ X86_INS_VPMOVSXBD, "vpmovsxbd" },
	{ X86_INS_VPMOVSXBQ, "vpmovsxbq" },
	{ X86_INS_VPMOVSXBW, "vpmovsxbw" },
	{ X86_INS_VPMOVSXDQ, "vpmovsxdq" },
	{ X86_INS_VPMOVSXWD, "vpmovsxwd" },
	{ X86_INS_VPMOVSXWQ, "vpmovsxwq" },
	{ X86_INS_VPMOVUSDB, "vpmovusdb" },
	{ X86_INS_VPMOVUSDW, "vpmovusdw" },
	{ X86_INS_VPMOVUSQB, "vpmovusqb" },
	{ X86_INS_VPMOVUSQD, "vpmovusqd" },
	{ X86_INS_VPMOVUSQW, "vpmovusqw" },
	{ X86_INS_VPMOVZXBD, "vpmovzxbd" },
	{ X86_INS_VPMOVZXBQ, "vpmovzxbq" },
	{ X86_INS_VPMOVZXBW, "vpmovzxbw" },
	{ X86_INS_VPMOVZXDQ, "vpmovzxdq" },
	{ X86_INS_VPMOVZXWD, "vpmovzxwd" },
	{ X86_INS_VPMOVZXWQ, "vpmovzxwq" },
	{ X86_INS_VPMULDQ, "vpmuldq" },
	{ X86_INS_VPMULHRSW, "vpmulhrsw" },
	{ X86_INS_VPMULHUW, "vpmulhuw" },
	{ X86_INS_VPMULHW, "vpmulhw" },
	{ X86_INS_VPMULLD, "vpmulld" },
	{ X86_INS_VPMULLQ, "vpmullq" },
	{ X86_INS_VPMULLW, "vpmullw" },
	{ X86_INS_VPMULUDQ, "vpmuludq" },
	{ X86_INS_VPORD, "vpord" },
	{ X86_INS_VPORQ, "vporq" },
	{ X86_INS_VPOR, "vpor" },
	{ X86_INS_VPPERM, "vpperm" },
	{ X86_INS_VPROTB, "vprotb" },
	{ X86_INS_VPROTD, "vprotd" },
	{ X86_INS_VPROTQ, "vprotq" },
	{ X86_INS_VPROTW, "vprotw" },
	{ X86_INS_VPSADBW, "vpsadbw" },
	{ X86_INS_VPSCATTERDD, "vpscatterdd" },
	{ X86_INS_VPSCATTERDQ, "vpscatterdq" },
	{ X86_INS_VPSCATTERQD, "vpscatterqd" },
	{ X86_INS_VPSCATTERQQ, "vpscatterqq" },
	{ X86_INS_VPSHAB, "vpshab" },
	{ X86_INS_VPSHAD, "vpshad" },
	{ X86_INS_VPSHAQ, "vpshaq" },
	{ X86_INS_VPSHAW, "vpshaw" },
	{ X86_INS_VPSHLB, "vpshlb" },
	{ X86_INS_VPSHLD, "vpshld" },
	{ X86_INS_VPSHLQ, "vpshlq" },
	{ X86_INS_VPSHLW, "vpshlw" },
	{ X86_INS_VPSHUFB, "vpshufb" },
	{ X86_INS_VPSHUFD, "vpshufd" },
	{ X86_INS_VPSHUFHW, "vpshufhw" },
	{ X86_INS_VPSHUFLW, "vpshuflw" },
	{ X86_INS_VPSIGNB, "vpsignb" },
	{ X86_INS_VPSIGND, "vpsignd" },
	{ X86_INS_VPSIGNW, "vpsignw" },
	{ X86_INS_VPSLLDQ, "vpslldq" },
	{ X86_INS_VPSLLD, "vpslld" },
	{ X86_INS_VPSLLQ, "vpsllq" },
	{ X86_INS_VPSLLVD, "vpsllvd" },
	{ X86_INS_VPSLLVQ, "vpsllvq" },
	{ X86_INS_VPSLLW, "vpsllw" },
	{ X86_INS_VPSRAD, "vpsrad" },
	{ X86_INS_VPSRAQ, "vpsraq" },
	{ X86_INS_VPSRAVD, "vpsravd" },
	{ X86_INS_VPSRAVQ, "vpsravq" },
	{ X86_INS_VPSRAW, "vpsraw" },
	{ X86_INS_VPSRLDQ, "vpsrldq" },
	{ X86_INS_VPSRLD, "vpsrld" },
	{ X86_INS_VPSRLQ, "vpsrlq" },
	{ X86_INS_VPSRLVD, "vpsrlvd" },
	{ X86_INS_VPSRLVQ, "vpsrlvq" },
	{ X86_INS_VPSRLW, "vpsrlw" },
	{ X86_INS_VPSUBB, "vpsubb" },
	{ X86_INS_VPSUBD, "vpsubd" },
	{ X86_INS_VPSUBQ, "vpsubq" },
	{ X86_INS_VPSUBSB, "vpsubsb" },
	{ X86_INS_VPSUBSW, "vpsubsw" },
	{ X86_INS_VPSUBUSB, "vpsubusb" },
	{ X86_INS_VPSUBUSW, "vpsubusw" },
	{ X86_INS_VPSUBW, "vpsubw" },
	{ X86_INS_VPTESTMD, "vptestmd" },
	{ X86_INS_VPTESTMQ, "vptestmq" },
	{ X86_INS_VPTESTNMD, "vptestnmd" },
	{ X86_INS_VPTESTNMQ, "vptestnmq" },
	{ X86_INS_VPTEST, "vptest" },
	{ X86_INS_VPUNPCKHBW, "vpunpckhbw" },
	{ X86_INS_VPUNPCKHDQ, "vpunpckhdq" },
	{ X86_INS_VPUNPCKHQDQ, "vpunpckhqdq" },
	{ X86_INS_VPUNPCKHWD, "vpunpckhwd" },
	{ X86_INS_VPUNPCKLBW, "vpunpcklbw" },
	{ X86_INS_VPUNPCKLDQ, "vpunpckldq" },
	{ X86_INS_VPUNPCKLQDQ, "vpunpcklqdq" },
	{ X86_INS_VPUNPCKLWD, "vpunpcklwd" },
	{ X86_INS_VPXORD, "vpxord" },
	{ X86_INS_VPXORQ, "vpxorq" },
	{ X86_INS_VPXOR, "vpxor" },
	{ X86_INS_VRCP14PD, "vrcp14pd" },
	{ X86_INS_VRCP14PS, "vrcp14ps" },
	{ X86_INS_VRCP14SD, "vrcp14sd" },
	{ X86_INS_VRCP14SS, "vrcp14ss" },
	{ X86_INS_VRCP28PD, "vrcp28pd" },
	{ X86_INS_VRCP28PS, "vrcp28ps" },
	{ X86_INS_VRCP28SD, "vrcp28sd" },
	{ X86_INS_VRCP28SS, "vrcp28ss" },
	{ X86_INS_VRCPPS, "vrcpps" },
	{ X86_INS_VRCPSS, "vrcpss" },
	{ X86_INS_VRNDSCALEPD, "vrndscalepd" },
	{ X86_INS_VRNDSCALEPS, "vrndscaleps" },
	{ X86_INS_VRNDSCALESD, "vrndscalesd" },
	{ X86_INS_VRNDSCALESS, "vrndscaless" },
	{ X86_INS_VROUNDPD, "vroundpd" },
	{ X86_INS_VROUNDPS, "vroundps" },
	{ X86_INS_VROUNDSD, "vroundsd" },
	{ X86_INS_VROUNDSS, "vroundss" },
	{ X86_INS_VRSQRT14PD, "vrsqrt14pd" },
	{ X86_INS_VRSQRT14PS, "vrsqrt14ps" },
	{ X86_INS_VRSQRT14SD, "vrsqrt14sd" },
	{ X86_INS_VRSQRT14SS, "vrsqrt14ss" },
	{ X86_INS_VRSQRT28PD, "vrsqrt28pd" },
	{ X86_INS_VRSQRT28PS, "vrsqrt28ps" },
	{ X86_INS_VRSQRT28SD, "vrsqrt28sd" },
	{ X86_INS_VRSQRT28SS, "vrsqrt28ss" },
	{ X86_INS_VRSQRTPS, "vrsqrtps" },
	{ X86_INS_VRSQRTSS, "vrsqrtss" },
	{ X86_INS_VSCATTERDPD, "vscatterdpd" },
	{ X86_INS_VSCATTERDPS, "vscatterdps" },
	{ X86_INS_VSCATTERPF0DPD, "vscatterpf0dpd" },
	{ X86_INS_VSCATTERPF0DPS, "vscatterpf0dps" },
	{ X86_INS_VSCATTERPF0QPD, "vscatterpf0qpd" },
	{ X86_INS_VSCATTERPF0QPS, "vscatterpf0qps" },
	{ X86_INS_VSCATTERPF1DPD, "vscatterpf1dpd" },
	{ X86_INS_VSCATTERPF1DPS, "vscatterpf1dps" },
	{ X86_INS_VSCATTERPF1QPD, "vscatterpf1qpd" },
	{ X86_INS_VSCATTERPF1QPS, "vscatterpf1qps" },
	{ X86_INS_VSCATTERQPD, "vscatterqpd" },
	{ X86_INS_VSCATTERQPS, "vscatterqps" },
	{ X86_INS_VSHUFPD, "vshufpd" },
	{ X86_INS_VSHUFPS, "vshufps" },
	{ X86_INS_VSQRTPD, "vsqrtpd" },
	{ X86_INS_VSQRTPS, "vsqrtps" },
	{ X86_INS_VSQRTSD, "vsqrtsd" },
	{ X86_INS_VSQRTSS, "vsqrtss" },
	{ X86_INS_VSTMXCSR, "vstmxcsr" },
	{ X86_INS_VSUBPD, "vsubpd" },
	{ X86_INS_VSUBPS, "vsubps" },
	{ X86_INS_VSUBSD, "vsubsd" },
	{ X86_INS_VSUBSS, "vsubss" },
	{ X86_INS_VTESTPD, "vtestpd" },
	{ X86_INS_VTESTPS, "vtestps" },
	{ X86_INS_VUNPCKHPD, "vunpckhpd" },
	{ X86_INS_VUNPCKHPS, "vunpckhps" },
	{ X86_INS_VUNPCKLPD, "vunpcklpd" },
	{ X86_INS_VUNPCKLPS, "vunpcklps" },
	{ X86_INS_VZEROALL, "vzeroall" },
	{ X86_INS_VZEROUPPER, "vzeroupper" },
	{ X86_INS_WAIT, "wait" },
	{ X86_INS_WBINVD, "wbinvd" },
	{ X86_INS_WRFSBASE, "wrfsbase" },
	{ X86_INS_WRGSBASE, "wrgsbase" },
	{ X86_INS_WRMSR, "wrmsr" },
	{ X86_INS_XABORT, "xabort" },
	{ X86_INS_XACQUIRE, "xacquire" },
	{ X86_INS_XBEGIN, "xbegin" },
	{ X86_INS_XCHG, "xchg" },
	{ X86_INS_XCRYPTCBC, "xcryptcbc" },
	{ X86_INS_XCRYPTCFB, "xcryptcfb" },
	{ X86_INS_XCRYPTCTR, "xcryptctr" },
	{ X86_INS_XCRYPTECB, "xcryptecb" },
	{ X86_INS_XCRYPTOFB, "xcryptofb" },
	{ X86_INS_XEND, "xend" },
	{ X86_INS_XGETBV, "xgetbv" },
	{ X86_INS_XLATB, "xlatb" },
	{ X86_INS_XRELEASE, "xrelease" },
	{ X86_INS_XRSTOR, "xrstor" },
	{ X86_INS_XRSTOR64, "xrstor64" },
	{ X86_INS_XRSTORS, "xrstors" },
	{ X86_INS_XRSTORS64, "xrstors64" },
	{ X86_INS_XSAVE, "xsave" },
	{ X86_INS_XSAVE64, "xsave64" },
	{ X86_INS_XSAVEC, "xsavec" },
	{ X86_INS_XSAVEC64, "xsavec64" },
	{ X86_INS_XSAVEOPT, "xsaveopt" },
	{ X86_INS_XSAVEOPT64, "xsaveopt64" },
	{ X86_INS_XSAVES, "xsaves" },
	{ X86_INS_XSAVES64, "xsaves64" },
	{ X86_INS_XSETBV, "xsetbv" },
	{ X86_INS_XSHA1, "xsha1" },
	{ X86_INS_XSHA256, "xsha256" },
	{ X86_INS_XSTORE, "xstore" },
	{ X86_INS_XTEST, "xtest" },
	{ X86_INS_FDISI8087_NOP, "fdisi8087_nop" },
	{ X86_INS_FENI8087_NOP, "feni8087_nop" },

	// pseudo instructions
	{ X86_INS_CMPSS, "cmpss" },
	{ X86_INS_CMPEQSS, "cmpeqss" },
	{ X86_INS_CMPLTSS, "cmpltss" },
	{ X86_INS_CMPLESS, "cmpless" },
	{ X86_INS_CMPUNORDSS, "cmpunordss" },
	{ X86_INS_CMPNEQSS, "cmpneqss" },
	{ X86_INS_CMPNLTSS, "cmpnltss" },
	{ X86_INS_CMPNLESS, "cmpnless" },
	{ X86_INS_CMPORDSS, "cmpordss" },

	{ X86_INS_CMPSD, "cmpsd" },
	{ X86_INS_CMPEQSD, "cmpeqsd" },
	{ X86_INS_CMPLTSD, "cmpltsd" },
	{ X86_INS_CMPLESD, "cmplesd" },
	{ X86_INS_CMPUNORDSD, "cmpunordsd" },
	{ X86_INS_CMPNEQSD, "cmpneqsd" },
	{ X86_INS_CMPNLTSD, "cmpnltsd" },
	{ X86_INS_CMPNLESD, "cmpnlesd" },
	{ X86_INS_CMPORDSD, "cmpordsd" },

	{ X86_INS_CMPPS, "cmpps" },
	{ X86_INS_CMPEQPS, "cmpeqps" },
	{ X86_INS_CMPLTPS, "cmpltps" },
	{ X86_INS_CMPLEPS, "cmpleps" },
	{ X86_INS_CMPUNORDPS, "cmpunordps" },
	{ X86_INS_CMPNEQPS, "cmpneqps" },
	{ X86_INS_CMPNLTPS, "cmpnltps" },
	{ X86_INS_CMPNLEPS, "cmpnleps" },
	{ X86_INS_CMPORDPS, "cmpordps" },

	{ X86_INS_CMPPD, "cmppd" },
	{ X86_INS_CMPEQPD, "cmpeqpd" },
	{ X86_INS_CMPLTPD, "cmpltpd" },
	{ X86_INS_CMPLEPD, "cmplepd" },
	{ X86_INS_CMPUNORDPD, "cmpunordpd" },
	{ X86_INS_CMPNEQPD, "cmpneqpd" },
	{ X86_INS_CMPNLTPD, "cmpnltpd" },
	{ X86_INS_CMPNLEPD, "cmpnlepd" },
	{ X86_INS_CMPORDPD, "cmpordpd" },

	{ X86_INS_VCMPSS, "vcmpss" },
	{ X86_INS_VCMPEQSS, "vcmpeqss" },
	{ X86_INS_VCMPLTSS, "vcmpltss" },
	{ X86_INS_VCMPLESS, "vcmpless" },
	{ X86_INS_VCMPUNORDSS, "vcmpunordss" },
	{ X86_INS_VCMPNEQSS, "vcmpneqss" },
	{ X86_INS_VCMPNLTSS, "vcmpnltss" },
	{ X86_INS_VCMPNLESS, "vcmpnless" },
	{ X86_INS_VCMPORDSS, "vcmpordss" },
	{ X86_INS_VCMPEQ_UQSS, "vcmpeq_uqss" },
	{ X86_INS_VCMPNGESS, "vcmpngess" },
	{ X86_INS_VCMPNGTSS, "vcmpngtss" },
	{ X86_INS_VCMPFALSESS, "vcmpfalsess" },
	{ X86_INS_VCMPNEQ_OQSS, "vcmpneq_oqss" },
	{ X86_INS_VCMPGESS, "vcmpgess" },
	{ X86_INS_VCMPGTSS, "vcmpgtss" },
	{ X86_INS_VCMPTRUESS, "vcmptruess" },
	{ X86_INS_VCMPEQ_OSSS, "vcmpeq_osss" },
	{ X86_INS_VCMPLT_OQSS, "vcmplt_oqss" },
	{ X86_INS_VCMPLE_OQSS, "vcmple_oqss" },
	{ X86_INS_VCMPUNORD_SSS, "vcmpunord_sss" },
	{ X86_INS_VCMPNEQ_USSS, "vcmpneq_usss" },
	{ X86_INS_VCMPNLT_UQSS, "vcmpnlt_uqss" },
	{ X86_INS_VCMPNLE_UQSS, "vcmpnle_uqss" },
	{ X86_INS_VCMPORD_SSS, "vcmpord_sss" },
	{ X86_INS_VCMPEQ_USSS, "vcmpeq_usss" },
	{ X86_INS_VCMPNGE_UQSS, "vcmpnge_uqss" },
	{ X86_INS_VCMPNGT_UQSS, "vcmpngt_uqss" },
	{ X86_INS_VCMPFALSE_OSSS, "vcmpfalse_osss" },
	{ X86_INS_VCMPNEQ_OSSS, "vcmpneq_osss" },
	{ X86_INS_VCMPGE_OQSS, "vcmpge_oqss" },
	{ X86_INS_VCMPGT_OQSS, "vcmpgt_oqss" },
	{ X86_INS_VCMPTRUE_USSS, "vcmptrue_usss" },

	{ X86_INS_VCMPSD, "vcmpsd" },
	{ X86_INS_VCMPEQSD, "vcmpeqsd" },
	{ X86_INS_VCMPLTSD, "vcmpltsd" },
	{ X86_INS_VCMPLESD, "vcmplesd" },
	{ X86_INS_VCMPUNORDSD, "vcmpunordsd" },
	{ X86_INS_VCMPNEQSD, "vcmpneqsd" },
	{ X86_INS_VCMPNLTSD, "vcmpnltsd" },
	{ X86_INS_VCMPNLESD, "vcmpnlesd" },
	{ X86_INS_VCMPORDSD, "vcmpordsd" },
	{ X86_INS_VCMPEQ_UQSD, "vcmpeq_uqsd" },
	{ X86_INS_VCMPNGESD, "vcmpngesd" },
	{ X86_INS_VCMPNGTSD, "vcmpngtsd" },
	{ X86_INS_VCMPFALSESD, "vcmpfalsesd" },
	{ X86_INS_VCMPNEQ_OQSD, "vcmpneq_oqsd" },
	{ X86_INS_VCMPGESD, "vcmpgesd" },
	{ X86_INS_VCMPGTSD, "vcmpgtsd" },
	{ X86_INS_VCMPTRUESD, "vcmptruesd" },
	{ X86_INS_VCMPEQ_OSSD, "vcmpeq_ossd" },
	{ X86_INS_VCMPLT_OQSD, "vcmplt_oqsd" },
	{ X86_INS_VCMPLE_OQSD, "vcmple_oqsd" },
	{ X86_INS_VCMPUNORD_SSD, "vcmpunord_ssd" },
	{ X86_INS_VCMPNEQ_USSD, "vcmpneq_ussd" },
	{ X86_INS_VCMPNLT_UQSD, "vcmpnlt_uqsd" },
	{ X86_INS_VCMPNLE_UQSD, "vcmpnle_uqsd" },
	{ X86_INS_VCMPORD_SSD, "vcmpord_ssd" },
	{ X86_INS_VCMPEQ_USSD, "vcmpeq_ussd" },
	{ X86_INS_VCMPNGE_UQSD, "vcmpnge_uqsd" },
	{ X86_INS_VCMPNGT_UQSD, "vcmpngt_uqsd" },
	{ X86_INS_VCMPFALSE_OSSD, "vcmpfalse_ossd" },
	{ X86_INS_VCMPNEQ_OSSD, "vcmpneq_ossd" },
	{ X86_INS_VCMPGE_OQSD, "vcmpge_oqsd" },
	{ X86_INS_VCMPGT_OQSD, "vcmpgt_oqsd" },
	{ X86_INS_VCMPTRUE_USSD, "vcmptrue_ussd" },

	{ X86_INS_VCMPPS, "vcmpps" },
	{ X86_INS_VCMPEQPS, "vcmpeqps" },
	{ X86_INS_VCMPLTPS, "vcmpltps" },
	{ X86_INS_VCMPLEPS, "vcmpleps" },
	{ X86_INS_VCMPUNORDPS, "vcmpunordps" },
	{ X86_INS_VCMPNEQPS, "vcmpneqps" },
	{ X86_INS_VCMPNLTPS, "vcmpnltps" },
	{ X86_INS_VCMPNLEPS, "vcmpnleps" },
	{ X86_INS_VCMPORDPS, "vcmpordps" },
	{ X86_INS_VCMPEQ_UQPS, "vcmpeq_uqps" },
	{ X86_INS_VCMPNGEPS, "vcmpngeps" },
	{ X86_INS_VCMPNGTPS, "vcmpngtps" },
	{ X86_INS_VCMPFALSEPS, "vcmpfalseps" },
	{ X86_INS_VCMPNEQ_OQPS, "vcmpneq_oqps" },
	{ X86_INS_VCMPGEPS, "vcmpgeps" },
	{ X86_INS_VCMPGTPS, "vcmpgtps" },
	{ X86_INS_VCMPTRUEPS, "vcmptrueps" },
	{ X86_INS_VCMPEQ_OSPS, "vcmpeq_osps" },
	{ X86_INS_VCMPLT_OQPS, "vcmplt_oqps" },
	{ X86_INS_VCMPLE_OQPS, "vcmple_oqps" },
	{ X86_INS_VCMPUNORD_SPS, "vcmpunord_sps" },
	{ X86_INS_VCMPNEQ_USPS, "vcmpneq_usps" },
	{ X86_INS_VCMPNLT_UQPS, "vcmpnlt_uqps" },
	{ X86_INS_VCMPNLE_UQPS, "vcmpnle_uqps" },
	{ X86_INS_VCMPORD_SPS, "vcmpord_sps" },
	{ X86_INS_VCMPEQ_USPS, "vcmpeq_usps" },
	{ X86_INS_VCMPNGE_UQPS, "vcmpnge_uqps" },
	{ X86_INS_VCMPNGT_UQPS, "vcmpngt_uqps" },
	{ X86_INS_VCMPFALSE_OSPS, "vcmpfalse_osps" },
	{ X86_INS_VCMPNEQ_OSPS, "vcmpneq_osps" },
	{ X86_INS_VCMPGE_OQPS, "vcmpge_oqps" },
	{ X86_INS_VCMPGT_OQPS, "vcmpgt_oqps" },
	{ X86_INS_VCMPTRUE_USPS, "vcmptrue_usps" },

	{ X86_INS_VCMPPD, "vcmppd" },
	{ X86_INS_VCMPEQPD, "vcmpeqpd" },
	{ X86_INS_VCMPLTPD, "vcmpltpd" },
	{ X86_INS_VCMPLEPD, "vcmplepd" },
	{ X86_INS_VCMPUNORDPD, "vcmpunordpd" },
	{ X86_INS_VCMPNEQPD, "vcmpneqpd" },
	{ X86_INS_VCMPNLTPD, "vcmpnltpd" },
	{ X86_INS_VCMPNLEPD, "vcmpnlepd" },
	{ X86_INS_VCMPORDPD, "vcmpordpd" },
	{ X86_INS_VCMPEQ_UQPD, "vcmpeq_uqpd" },
	{ X86_INS_VCMPNGEPD, "vcmpngepd" },
	{ X86_INS_VCMPNGTPD, "vcmpngtpd" },
	{ X86_INS_VCMPFALSEPD, "vcmpfalsepd" },
	{ X86_INS_VCMPNEQ_OQPD, "vcmpneq_oqpd" },
	{ X86_INS_VCMPGEPD, "vcmpgepd" },
	{ X86_INS_VCMPGTPD, "vcmpgtpd" },
	{ X86_INS_VCMPTRUEPD, "vcmptruepd" },
	{ X86_INS_VCMPEQ_OSPD, "vcmpeq_ospd" },
	{ X86_INS_VCMPLT_OQPD, "vcmplt_oqpd" },
	{ X86_INS_VCMPLE_OQPD, "vcmple_oqpd" },
	{ X86_INS_VCMPUNORD_SPD, "vcmpunord_spd" },
	{ X86_INS_VCMPNEQ_USPD, "vcmpneq_uspd" },
	{ X86_INS_VCMPNLT_UQPD, "vcmpnlt_uqpd" },
	{ X86_INS_VCMPNLE_UQPD, "vcmpnle_uqpd" },
	{ X86_INS_VCMPORD_SPD, "vcmpord_spd" },
	{ X86_INS_VCMPEQ_USPD, "vcmpeq_uspd" },
	{ X86_INS_VCMPNGE_UQPD, "vcmpnge_uqpd" },
	{ X86_INS_VCMPNGT_UQPD, "vcmpngt_uqpd" },
	{ X86_INS_VCMPFALSE_OSPD, "vcmpfalse_ospd" },
	{ X86_INS_VCMPNEQ_OSPD, "vcmpneq_ospd" },
	{ X86_INS_VCMPGE_OQPD, "vcmpge_oqpd" },
	{ X86_INS_VCMPGT_OQPD, "vcmpgt_oqpd" },
	{ X86_INS_VCMPTRUE_USPD, "vcmptrue_uspd" },

	{ X86_INS_UD0, "ud0" },
	{ X86_INS_ENDBR32, "endbr32" },
	{ X86_INS_ENDBR64, "endbr64" },
};
#endif

const char *X86_insn_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	if (id >= X86_INS_ENDING)
		return NULL;

	return insn_name_maps[id].name;
#else
	return NULL;
#endif
}

#ifndef CAPSTONE_DIET
static const name_map group_name_maps[] = {
	// generic groups
	{ X86_GRP_INVALID, NULL },
	{ X86_GRP_JUMP,	"jump" },
	{ X86_GRP_CALL,	"call" },
	{ X86_GRP_RET, "ret" },
	{ X86_GRP_INT, "int" },
	{ X86_GRP_IRET,	"iret" },
	{ X86_GRP_PRIVILEGE, "privilege" },
	{ X86_GRP_BRANCH_RELATIVE, "branch_relative" },

	// architecture-specific groups
	{ X86_GRP_VM, "vm" },
	{ X86_GRP_3DNOW, "3dnow" },
	{ X86_GRP_AES, "aes" },
	{ X86_GRP_ADX, "adx" },
	{ X86_GRP_AVX, "avx" },
	{ X86_GRP_AVX2, "avx2" },
	{ X86_GRP_AVX512, "avx512" },
	{ X86_GRP_BMI, "bmi" },
	{ X86_GRP_BMI2, "bmi2" },
	{ X86_GRP_CMOV, "cmov" },
	{ X86_GRP_F16C, "fc16" },
	{ X86_GRP_FMA, "fma" },
	{ X86_GRP_FMA4, "fma4" },
	{ X86_GRP_FSGSBASE, "fsgsbase" },
	{ X86_GRP_HLE, "hle" },
	{ X86_GRP_MMX, "mmx" },
	{ X86_GRP_MODE32, "mode32" },
	{ X86_GRP_MODE64, "mode64" },
	{ X86_GRP_RTM, "rtm" },
	{ X86_GRP_SHA, "sha" },
	{ X86_GRP_SSE1, "sse1" },
	{ X86_GRP_SSE2, "sse2" },
	{ X86_GRP_SSE3, "sse3" },
	{ X86_GRP_SSE41, "sse41" },
	{ X86_GRP_SSE42, "sse42" },
	{ X86_GRP_SSE4A, "sse4a" },
	{ X86_GRP_SSSE3, "ssse3" },
	{ X86_GRP_PCLMUL, "pclmul" },
	{ X86_GRP_XOP, "xop" },
	{ X86_GRP_CDI, "cdi" },
	{ X86_GRP_ERI, "eri" },
	{ X86_GRP_TBM, "tbm" },
	{ X86_GRP_16BITMODE, "16bitmode" },
	{ X86_GRP_NOT64BITMODE, "not64bitmode" },
	{ X86_GRP_SGX,	"sgx" },
	{ X86_GRP_DQI,	"dqi" },
	{ X86_GRP_BWI,	"bwi" },
	{ X86_GRP_PFI,	"pfi" },
	{ X86_GRP_VLX,	"vlx" },
	{ X86_GRP_SMAP,	"smap" },
	{ X86_GRP_NOVLX, "novlx" },
	{ X86_GRP_FPU, "fpu" },
};
#endif

const char *X86_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(group_name_maps, ARR_SIZE(group_name_maps), id);
#else
	return NULL;
#endif
}

#define GET_INSTRINFO_ENUM
#ifdef CAPSTONE_X86_REDUCE
#include "X86GenInstrInfo_reduce.inc"
#else
#include "X86GenInstrInfo.inc"
#endif

#ifndef CAPSTONE_X86_REDUCE
static const insn_map insns[] = {	// full x86 instructions
	// dummy item
	{
		0, 0,
#ifndef CAPSTONE_DIET
		{ 0 }, { 0 }, { 0 }, 0, 0
#endif
	},

#include "X86MappingInsn.inc"
};
#else	// X86 reduce (defined CAPSTONE_X86_REDUCE)
static insn_map insns[] = {	// reduce x86 instructions
	// dummy item
	{
		0, 0,
#ifndef CAPSTONE_DIET
		{ 0 }, { 0 }, { 0 }, 0, 0
#endif
	},

#include "X86MappingInsn_reduce.inc"
};
#endif

#ifndef CAPSTONE_DIET
// replace r1 = r2
static void arr_replace(uint16_t *arr, uint8_t max, x86_reg r1, x86_reg r2)
{
	uint8_t i;

	for(i = 0; i < max; i++) {
		if (arr[i] == r1) {
			arr[i] = r2;
			break;
		}
	}
}
#endif

// given internal insn id, return public instruction info
void X86_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id)
{
	int i = insn_find(insns, ARR_SIZE(insns), id, &h->insn_cache);
	if (i != 0) {
		insn->id = insns[i].mapid;

		if (h->detail) {
#ifndef CAPSTONE_DIET
			memcpy(insn->detail->regs_read, insns[i].regs_use, sizeof(insns[i].regs_use));
			insn->detail->regs_read_count = (uint8_t)count_positive(insns[i].regs_use);

			// special cases when regs_write[] depends on arch
			switch(id) {
				default:
					memcpy(insn->detail->regs_write, insns[i].regs_mod, sizeof(insns[i].regs_mod));
					insn->detail->regs_write_count = (uint8_t)count_positive(insns[i].regs_mod);
					break;
				case X86_RDTSC:
					if (h->mode == CS_MODE_64) {
						memcpy(insn->detail->regs_write, insns[i].regs_mod, sizeof(insns[i].regs_mod));
						insn->detail->regs_write_count = (uint8_t)count_positive(insns[i].regs_mod);
					} else {
						insn->detail->regs_write[0] = X86_REG_EAX;
						insn->detail->regs_write[1] = X86_REG_EDX;
						insn->detail->regs_write_count = 2;
					}
					break;
				case X86_RDTSCP:
					if (h->mode == CS_MODE_64) {
						memcpy(insn->detail->regs_write, insns[i].regs_mod, sizeof(insns[i].regs_mod));
						insn->detail->regs_write_count = (uint8_t)count_positive(insns[i].regs_mod);
					} else {
						insn->detail->regs_write[0] = X86_REG_EAX;
						insn->detail->regs_write[1] = X86_REG_ECX;
						insn->detail->regs_write[2] = X86_REG_EDX;
						insn->detail->regs_write_count = 3;
					}
					break;
			}

			switch(insn->id) {
				default:
					break;

				case X86_INS_LOOP:
				case X86_INS_LOOPE:
				case X86_INS_LOOPNE:
					switch(h->mode) {
						default: break;
						case CS_MODE_16:
								 insn->detail->regs_read[0] = X86_REG_CX;
								 insn->detail->regs_read_count = 1;
								 insn->detail->regs_write[0] = X86_REG_CX;
								 insn->detail->regs_write_count = 1;
								 break;
						case CS_MODE_32:
								 insn->detail->regs_read[0] = X86_REG_ECX;
								 insn->detail->regs_read_count = 1;
								 insn->detail->regs_write[0] = X86_REG_ECX;
								 insn->detail->regs_write_count = 1;
								 break;
						case CS_MODE_64:
								 insn->detail->regs_read[0] = X86_REG_RCX;
								 insn->detail->regs_read_count = 1;
								 insn->detail->regs_write[0] = X86_REG_RCX;
								 insn->detail->regs_write_count = 1;
								 break;
					}

					// LOOPE & LOOPNE also read EFLAGS
					if (insn->id != X86_INS_LOOP) {
						insn->detail->regs_read[1] = X86_REG_EFLAGS;
						insn->detail->regs_read_count = 2;
					}

					break;

				case X86_INS_LODSB:
				case X86_INS_LODSD:
				case X86_INS_LODSQ:
				case X86_INS_LODSW:
					switch(h->mode) {
						default:
							break;
						case CS_MODE_16:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_ESI, X86_REG_SI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_ESI, X86_REG_SI);
							break;
						case CS_MODE_64:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_ESI, X86_REG_RSI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_ESI, X86_REG_RSI);
							break;
					}
					break;

				case X86_INS_SCASB:
				case X86_INS_SCASW:
				case X86_INS_SCASQ:
				case X86_INS_STOSB:
				case X86_INS_STOSD:
				case X86_INS_STOSQ:
				case X86_INS_STOSW:
					switch(h->mode) {
						default:
							break;
						case CS_MODE_16:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_EDI, X86_REG_DI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_EDI, X86_REG_DI);
							break;
						case CS_MODE_64:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_EDI, X86_REG_RDI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_EDI, X86_REG_RDI);
							break;
					}
					break;

				case X86_INS_CMPSB:
				case X86_INS_CMPSD:
				case X86_INS_CMPSQ:
				case X86_INS_CMPSW:
				case X86_INS_MOVSB:
				case X86_INS_MOVSW:
				case X86_INS_MOVSD:
				case X86_INS_MOVSQ:
					switch(h->mode) {
						default:
							break;
						case CS_MODE_16:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_EDI, X86_REG_DI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_EDI, X86_REG_DI);
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_ESI, X86_REG_SI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_ESI, X86_REG_SI);
							break;
						case CS_MODE_64:
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_EDI, X86_REG_RDI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_EDI, X86_REG_RDI);
							arr_replace(insn->detail->regs_read, insn->detail->regs_read_count, X86_REG_ESI, X86_REG_RSI);
							arr_replace(insn->detail->regs_write, insn->detail->regs_write_count, X86_REG_ESI, X86_REG_RSI);
							break;
					}
					break;

				case X86_INS_RET:
					switch(h->mode) {
						case CS_MODE_16:
							insn->detail->regs_write[0] = X86_REG_SP;
							insn->detail->regs_read[0] = X86_REG_SP;
							break;
						case CS_MODE_32:
							insn->detail->regs_write[0] = X86_REG_ESP;
							insn->detail->regs_read[0] = X86_REG_ESP;
							break;
						default:	// 64-bit
							insn->detail->regs_write[0] = X86_REG_RSP;
							insn->detail->regs_read[0] = X86_REG_RSP;
							break;
					}
					insn->detail->regs_write_count = 1;
					insn->detail->regs_read_count = 1;
					break;
			}

			memcpy(insn->detail->groups, insns[i].groups, sizeof(insns[i].groups));
			insn->detail->groups_count = (uint8_t)count_positive8(insns[i].groups);

			if (insns[i].branch || insns[i].indirect_branch) {
				// this insn also belongs to JUMP group. add JUMP group
				insn->detail->groups[insn->detail->groups_count] = X86_GRP_JUMP;
				insn->detail->groups_count++;
			}

			switch (insns[i].id) {
				case X86_OUT8ir:
				case X86_OUT16ir:
				case X86_OUT32ir:
					if (insn->detail->x86.operands[0].imm == -78) {
						// Writing to port 0xb2 causes an SMI on most platforms
						// See: http://cs.gmu.edu/~tr-admin/papers/GMU-CS-TR-2011-8.pdf
						insn->detail->groups[insn->detail->groups_count] = X86_GRP_INT;
						insn->detail->groups_count++;
					}
					break;

				default:
					break;
			}
#endif
		}
	}
}

// map special instructions with accumulate registers.
// this is needed because LLVM embeds these register names into AsmStrs[],
// but not separately in operands
struct insn_reg {
	uint16_t insn;
	x86_reg reg;
	enum cs_ac_type access;
};

struct insn_reg2 {
	uint16_t insn;
	x86_reg reg1, reg2;
	enum cs_ac_type access1, access2;
};

static struct insn_reg insn_regs_att[] = {
	{ X86_INSB, X86_REG_DX },
	{ X86_INSW, X86_REG_DX },
	{ X86_INSL, X86_REG_DX },

	{ X86_MOV8o16a, X86_REG_AL },
	{ X86_MOV8o32a, X86_REG_AL },
	{ X86_MOV8o64a, X86_REG_AL },

	{ X86_MOV16o16a, X86_REG_AX },
	{ X86_MOV16o32a, X86_REG_AX },
	{ X86_MOV16o64a, X86_REG_AX },

	{ X86_MOV32o16a, X86_REG_EAX },
	{ X86_MOV32o32a, X86_REG_EAX },
	{ X86_MOV32o64a, X86_REG_EAX },

	{ X86_MOV64o32a, X86_REG_RAX },
	{ X86_MOV64o64a, X86_REG_RAX },

	{ X86_PUSHCS32, X86_REG_CS },
	{ X86_PUSHDS32, X86_REG_DS },
	{ X86_PUSHES32, X86_REG_ES },
	{ X86_PUSHFS32, X86_REG_FS },
	{ X86_PUSHGS32, X86_REG_GS },
	{ X86_PUSHSS32, X86_REG_SS },

	{ X86_PUSHFS64, X86_REG_FS },
	{ X86_PUSHGS64, X86_REG_GS },

	{ X86_PUSHCS16, X86_REG_CS },
	{ X86_PUSHDS16, X86_REG_DS },
	{ X86_PUSHES16, X86_REG_ES },
	{ X86_PUSHFS16, X86_REG_FS },
	{ X86_PUSHGS16, X86_REG_GS },
	{ X86_PUSHSS16, X86_REG_SS },

	{ X86_POPDS32, X86_REG_DS },
	{ X86_POPES32, X86_REG_ES },
	{ X86_POPFS32, X86_REG_FS },
	{ X86_POPGS32, X86_REG_GS },
	{ X86_POPSS32, X86_REG_SS },

	{ X86_POPFS64, X86_REG_FS },
	{ X86_POPGS64, X86_REG_GS },

	{ X86_POPDS16, X86_REG_DS },
	{ X86_POPES16, X86_REG_ES },
	{ X86_POPFS16, X86_REG_FS },
	{ X86_POPGS16, X86_REG_GS },
	{ X86_POPSS16, X86_REG_SS },

	{ X86_RCL32rCL, X86_REG_CL },
	{ X86_SHL8rCL, X86_REG_CL },
	{ X86_SHL16rCL, X86_REG_CL },
	{ X86_SHL32rCL, X86_REG_CL },
	{ X86_SHL64rCL, X86_REG_CL },
	{ X86_SAL8rCL, X86_REG_CL },
	{ X86_SAL16rCL, X86_REG_CL },
	{ X86_SAL32rCL, X86_REG_CL },
	{ X86_SAL64rCL, X86_REG_CL },
	{ X86_SHR8rCL, X86_REG_CL },
	{ X86_SHR16rCL, X86_REG_CL },
	{ X86_SHR32rCL, X86_REG_CL },
	{ X86_SHR64rCL, X86_REG_CL },
	{ X86_SAR8rCL, X86_REG_CL },
	{ X86_SAR16rCL, X86_REG_CL },
	{ X86_SAR32rCL, X86_REG_CL },
	{ X86_SAR64rCL, X86_REG_CL },
	{ X86_RCL8rCL, X86_REG_CL },
	{ X86_RCL16rCL, X86_REG_CL },
	{ X86_RCL32rCL, X86_REG_CL },
	{ X86_RCL64rCL, X86_REG_CL },
	{ X86_RCR8rCL, X86_REG_CL },
	{ X86_RCR16rCL, X86_REG_CL },
	{ X86_RCR32rCL, X86_REG_CL },
	{ X86_RCR64rCL, X86_REG_CL },
	{ X86_ROL8rCL, X86_REG_CL },
	{ X86_ROL16rCL, X86_REG_CL },
	{ X86_ROL32rCL, X86_REG_CL },
	{ X86_ROL64rCL, X86_REG_CL },
	{ X86_ROR8rCL, X86_REG_CL },
	{ X86_ROR16rCL, X86_REG_CL },
	{ X86_ROR32rCL, X86_REG_CL },
	{ X86_ROR64rCL, X86_REG_CL },
	{ X86_SHLD16rrCL, X86_REG_CL },
	{ X86_SHRD16rrCL, X86_REG_CL },
	{ X86_SHLD32rrCL, X86_REG_CL },
	{ X86_SHRD32rrCL, X86_REG_CL },
	{ X86_SHLD64rrCL, X86_REG_CL },
	{ X86_SHRD64rrCL, X86_REG_CL },
	{ X86_SHLD16mrCL, X86_REG_CL },
	{ X86_SHRD16mrCL, X86_REG_CL },
	{ X86_SHLD32mrCL, X86_REG_CL },
	{ X86_SHRD32mrCL, X86_REG_CL },
	{ X86_SHLD64mrCL, X86_REG_CL },
	{ X86_SHRD64mrCL, X86_REG_CL },

	{ X86_OUT8ir, X86_REG_AL },
	{ X86_OUT16ir, X86_REG_AX },
	{ X86_OUT32ir, X86_REG_EAX },

#ifndef CAPSTONE_X86_REDUCE
	{ X86_SKINIT, X86_REG_EAX },
	{ X86_VMRUN32, X86_REG_EAX },
	{ X86_VMRUN64, X86_REG_RAX },
	{ X86_VMLOAD32, X86_REG_EAX },
	{ X86_VMLOAD64, X86_REG_RAX },
	{ X86_VMSAVE32, X86_REG_EAX },
	{ X86_VMSAVE64, X86_REG_RAX },

	{ X86_FNSTSW16r, X86_REG_AX },

	{ X86_ADD_FrST0, X86_REG_ST0 },
	{ X86_SUB_FrST0, X86_REG_ST0 },
	{ X86_SUBR_FrST0, X86_REG_ST0 },
	{ X86_MUL_FrST0, X86_REG_ST0 },
	{ X86_DIV_FrST0, X86_REG_ST0 },
	{ X86_DIVR_FrST0, X86_REG_ST0 },
#endif
};

static struct insn_reg insn_regs_intel[] = {
	{ X86_OUTSB, X86_REG_DX, CS_AC_WRITE },
	{ X86_OUTSW, X86_REG_DX, CS_AC_WRITE },
	{ X86_OUTSL, X86_REG_DX, CS_AC_WRITE },

	{ X86_MOV8ao16, X86_REG_AL, CS_AC_WRITE },     // 16-bit A0 1020                  // mov     al, byte ptr [0x2010]
	{ X86_MOV8ao32, X86_REG_AL, CS_AC_WRITE },     // 32-bit A0 10203040              // mov     al, byte ptr [0x40302010]
	{ X86_MOV8ao64, X86_REG_AL, CS_AC_WRITE },     // 64-bit 66 A0 1020304050607080   // movabs  al, byte ptr [0x8070605040302010]

	{ X86_MOV16ao16, X86_REG_AX, CS_AC_WRITE },    // 16-bit A1 1020                  // mov     ax, word ptr [0x2010]
	{ X86_MOV16ao32, X86_REG_AX, CS_AC_WRITE },    // 32-bit A1 10203040              // mov     ax, word ptr [0x40302010]
	{ X86_MOV16ao64, X86_REG_AX, CS_AC_WRITE },    // 64-bit 66 A1 1020304050607080   // movabs  ax, word ptr [0x8070605040302010]

	{ X86_MOV32ao16, X86_REG_EAX, CS_AC_WRITE },   // 32-bit 67 A1 1020               // mov     eax, dword ptr [0x2010]
	{ X86_MOV32ao32, X86_REG_EAX, CS_AC_WRITE },   // 32-bit A1 10203040              // mov     eax, dword ptr [0x40302010]
	{ X86_MOV32ao64, X86_REG_EAX, CS_AC_WRITE },   // 64-bit A1 1020304050607080      // movabs  eax, dword ptr [0x8070605040302010]

	{ X86_MOV64ao32, X86_REG_RAX, CS_AC_WRITE },   // 64-bit 48 8B04 10203040         // mov     rax, qword ptr [0x40302010]
	{ X86_MOV64ao64, X86_REG_RAX, CS_AC_WRITE },   // 64-bit 48 A1 1020304050607080   // movabs  rax, qword ptr [0x8070605040302010]

	{ X86_LODSQ, X86_REG_RAX, CS_AC_WRITE },
	{ X86_OR32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SUB32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_TEST32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADD32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XCHG64ar, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_LODSB, X86_REG_AL, CS_AC_WRITE },
	{ X86_AND32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_IN16ri, X86_REG_AX, CS_AC_WRITE },
	{ X86_CMP64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XOR32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XCHG16ar, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_LODSW, X86_REG_AX, CS_AC_WRITE },
	{ X86_AND16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADC16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XCHG32ar64, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADC8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_CMP32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_AND8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_SCASW, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XOR8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_SUB16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_OR16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XCHG32ar, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SBB8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_SCASQ, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SBB32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XOR64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SUB64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADD64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_OR8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_TEST64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SBB16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_TEST8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_IN8ri, X86_REG_AL, CS_AC_WRITE },
	{ X86_TEST16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SCASL, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SUB8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADD8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_OR64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_SCASB, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_SBB64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADD16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_XOR16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_AND64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_LODSL, X86_REG_EAX, CS_AC_WRITE },
	{ X86_CMP8i8, X86_REG_AL, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADC64i32, X86_REG_RAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_CMP16i16, X86_REG_AX, CS_AC_WRITE | CS_AC_READ },
	{ X86_ADC32i32, X86_REG_EAX, CS_AC_WRITE | CS_AC_READ },
	{ X86_IN32ri, X86_REG_EAX, CS_AC_WRITE },

	{ X86_PUSHCS32, X86_REG_CS, CS_AC_READ },
	{ X86_PUSHDS32, X86_REG_DS, CS_AC_READ },
	{ X86_PUSHES32, X86_REG_ES, CS_AC_READ },
	{ X86_PUSHFS32, X86_REG_FS, CS_AC_READ },
	{ X86_PUSHGS32, X86_REG_GS, CS_AC_READ },
	{ X86_PUSHSS32, X86_REG_SS, CS_AC_READ },

	{ X86_PUSHFS64, X86_REG_FS, CS_AC_READ },
	{ X86_PUSHGS64, X86_REG_GS, CS_AC_READ },

	{ X86_PUSHCS16, X86_REG_CS, CS_AC_READ },
	{ X86_PUSHDS16, X86_REG_DS, CS_AC_READ },
	{ X86_PUSHES16, X86_REG_ES, CS_AC_READ },
	{ X86_PUSHFS16, X86_REG_FS, CS_AC_READ },
	{ X86_PUSHGS16, X86_REG_GS, CS_AC_READ },
	{ X86_PUSHSS16, X86_REG_SS, CS_AC_READ },

	{ X86_POPDS32, X86_REG_DS, CS_AC_WRITE },
	{ X86_POPES32, X86_REG_ES, CS_AC_WRITE },
	{ X86_POPFS32, X86_REG_FS, CS_AC_WRITE },
	{ X86_POPGS32, X86_REG_GS, CS_AC_WRITE },
	{ X86_POPSS32, X86_REG_SS, CS_AC_WRITE },

	{ X86_POPFS64, X86_REG_FS, CS_AC_WRITE },
	{ X86_POPGS64, X86_REG_GS, CS_AC_WRITE },

	{ X86_POPDS16, X86_REG_DS, CS_AC_WRITE },
	{ X86_POPES16, X86_REG_ES, CS_AC_WRITE },
	{ X86_POPFS16, X86_REG_FS, CS_AC_WRITE },
	{ X86_POPGS16, X86_REG_GS, CS_AC_WRITE },
	{ X86_POPSS16, X86_REG_SS, CS_AC_WRITE },

#ifndef CAPSTONE_X86_REDUCE
	{ X86_SKINIT, X86_REG_EAX, CS_AC_WRITE },
	{ X86_VMRUN32, X86_REG_EAX, CS_AC_WRITE },
	{ X86_VMRUN64, X86_REG_RAX, CS_AC_WRITE },
	{ X86_VMLOAD32, X86_REG_EAX, CS_AC_WRITE },
	{ X86_VMLOAD64, X86_REG_RAX, CS_AC_WRITE },
	{ X86_VMSAVE32, X86_REG_EAX, CS_AC_READ },
	{ X86_VMSAVE64, X86_REG_RAX, CS_AC_READ },

	{ X86_FNSTSW16r, X86_REG_AX, CS_AC_WRITE },

	{ X86_CMOVB_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVBE_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVE_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVP_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVNB_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVNBE_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVNE_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_CMOVNP_F, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FXCHST0r, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FXCHST0r_alt, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FCOMST0r, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FCOMPST0r, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FCOMPST0r_alt, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FPST0r, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FPST0r_alt, X86_REG_ST0, CS_AC_WRITE },
	{ X86_ST_FPNCEST0r, X86_REG_ST0, CS_AC_WRITE },
#endif
};

static struct insn_reg2 insn_regs_intel2[] = {
	{ X86_IN8rr, X86_REG_AL, X86_REG_DX, CS_AC_WRITE, CS_AC_READ },
	{ X86_IN16rr, X86_REG_AX, X86_REG_DX, CS_AC_WRITE, CS_AC_READ },
	{ X86_IN32rr, X86_REG_EAX, X86_REG_DX, CS_AC_WRITE, CS_AC_READ },

	{ X86_OUT8rr, X86_REG_DX, X86_REG_AL, CS_AC_READ, CS_AC_READ },
	{ X86_OUT16rr, X86_REG_DX, X86_REG_AX, CS_AC_READ, CS_AC_READ },
	{ X86_OUT32rr, X86_REG_DX, X86_REG_EAX, CS_AC_READ, CS_AC_READ },

	{ X86_INVLPGA32, X86_REG_EAX, X86_REG_ECX, CS_AC_READ, CS_AC_READ },
	{ X86_INVLPGA64, X86_REG_RAX, X86_REG_ECX, CS_AC_READ, CS_AC_READ },
};

static struct insn_reg insn_regs_intel_sorted [ARR_SIZE(insn_regs_intel)];

// Explicitly specified calling convention with CAPSTONE_API so that it is always
// compiled as __cdecl on MSVC and does not cause a compile error even when
// default calling convention is __stdcall (eg. capstone_static_winkernel project)
static int CAPSTONE_API regs_cmp(const void *a, const void *b)
{
	uint16_t l = ((struct insn_reg *)a)->insn;
	uint16_t r = ((struct insn_reg *)b)->insn;
	return (l - r);
}

// return register of given instruction id
// return 0 if not found
// this is to handle instructions embedding accumulate registers into AsmStrs[]
x86_reg X86_insn_reg_intel(unsigned int id, enum cs_ac_type *access)
{
	static bool intel_regs_sorted = false;
	unsigned int first = 0;
	unsigned int last = ARR_SIZE(insn_regs_intel) - 1;
	unsigned int mid;

	if (!intel_regs_sorted) {
		memcpy(insn_regs_intel_sorted, insn_regs_intel,
				sizeof(insn_regs_intel_sorted));
		qsort(insn_regs_intel_sorted,
				ARR_SIZE(insn_regs_intel_sorted),
				sizeof(struct insn_reg), regs_cmp);
		intel_regs_sorted = true;
	}

	if (insn_regs_intel_sorted[0].insn > id ||
			insn_regs_intel_sorted[last].insn < id) {
		return 0;
	}

	while (first <= last) {
		mid = (first + last) / 2;
		if (insn_regs_intel_sorted[mid].insn < id) {
			first = mid + 1;
		} else if (insn_regs_intel_sorted[mid].insn == id) {
			if (access) {
				*access = insn_regs_intel_sorted[mid].access;
			}
			return insn_regs_intel_sorted[mid].reg;
		} else {
			if (mid == 0)
				break;
			last = mid - 1;
		}
	}

	// not found
	return 0;
}

bool X86_insn_reg_intel2(unsigned int id, x86_reg *reg1, enum cs_ac_type *access1, x86_reg *reg2, enum cs_ac_type *access2)
{
	unsigned int i;

	for (i = 0; i < ARR_SIZE(insn_regs_intel2); i++) {
		if (insn_regs_intel2[i].insn == id) {
			*reg1 = insn_regs_intel2[i].reg1;
			*reg2 = insn_regs_intel2[i].reg2;
			if (access1)
				*access1 = insn_regs_intel2[i].access1;
			if (access2)
				*access2 = insn_regs_intel2[i].access2;
			return true;
		}
	}

	// not found
	return false;
}

// ATT just reuses Intel data, but with the order of registers reversed
bool X86_insn_reg_att2(unsigned int id, x86_reg *reg1, enum cs_ac_type *access1, x86_reg *reg2, enum cs_ac_type *access2)
{
	unsigned int i;

	for (i = 0; i < ARR_SIZE(insn_regs_intel2); i++) {
		if (insn_regs_intel2[i].insn == id) {
			// reverse order of Intel syntax registers
			*reg1 = insn_regs_intel2[i].reg2;
			*reg2 = insn_regs_intel2[i].reg1;
            if (access1)
                *access1 = insn_regs_intel2[i].access2;
            if (access2)
                *access2 = insn_regs_intel2[i].access1;
			return true;
		}
	}

	// not found
	return false;
}

x86_reg X86_insn_reg_att(unsigned int id, enum cs_ac_type *access)
{
	unsigned int i;

	for (i = 0; i < ARR_SIZE(insn_regs_att); i++) {
		if (insn_regs_att[i].insn == id) {
			if (access)
				*access = insn_regs_att[i].access;
			return insn_regs_att[i].reg;
		}
	}

	// not found
	return 0;
}

// given MCInst's id, find out if this insn is valid for REPNE prefix
static bool valid_repne(cs_struct *h, unsigned int opcode)
{
	unsigned int id;
	int i = insn_find(insns, ARR_SIZE(insns), opcode, &h->insn_cache);
	if (i != 0) {
		id = insns[i].mapid;
		switch(id) {
			default:
				return false;

			case X86_INS_CMPSB:
			case X86_INS_CMPSW:
			case X86_INS_CMPSQ:

			case X86_INS_SCASB:
			case X86_INS_SCASW:
			case X86_INS_SCASQ:

			case X86_INS_MOVSB:
			case X86_INS_MOVSW:
			case X86_INS_MOVSQ:

			case X86_INS_LODSB:
			case X86_INS_LODSW:
			case X86_INS_LODSD:
			case X86_INS_LODSQ:

			case X86_INS_STOSB:
			case X86_INS_STOSW:
			case X86_INS_STOSD:
			case X86_INS_STOSQ:

			case X86_INS_INSB:
			case X86_INS_INSW:
			case X86_INS_INSD:

			case X86_INS_OUTSB:
			case X86_INS_OUTSW:
			case X86_INS_OUTSD:

				return true;

			case X86_INS_MOVSD:
				if (opcode == X86_MOVSW) // REP MOVSB
					return true;
				return false;

			case X86_INS_CMPSD:
				if (opcode == X86_CMPSL) // REP CMPSD
					return true;
				return false;

			case X86_INS_SCASD:
				if (opcode == X86_SCASL) // REP SCASD
					return true;
				return false;
		}
	}

	// not found
	return false;
}

// given MCInst's id, find out if this insn is valid for BND prefix
// BND prefix is valid for CALL/JMP/RET
#ifndef CAPSTONE_DIET
static bool valid_bnd(cs_struct *h, unsigned int opcode)
{
	unsigned int id;
	int i = insn_find(insns, ARR_SIZE(insns), opcode, &h->insn_cache);
	if (i != 0) {
		id = insns[i].mapid;
		switch(id) {
			default:
				return false;

			case X86_INS_JAE:
			case X86_INS_JA:
			case X86_INS_JBE:
			case X86_INS_JB:
			case X86_INS_JCXZ:
			case X86_INS_JECXZ:
			case X86_INS_JE:
			case X86_INS_JGE:
			case X86_INS_JG:
			case X86_INS_JLE:
			case X86_INS_JL:
			case X86_INS_JMP:
			case X86_INS_JNE:
			case X86_INS_JNO:
			case X86_INS_JNP:
			case X86_INS_JNS:
			case X86_INS_JO:
			case X86_INS_JP:
			case X86_INS_JRCXZ:
			case X86_INS_JS:

			case X86_INS_CALL:
			case X86_INS_RET:
			case X86_INS_RETF:
			case X86_INS_RETFQ:
				return true;
		}
	}

	// not found
	return false;
}
#endif

// return true if the opcode is XCHG [mem]
static bool xchg_mem(unsigned int opcode)
{
	switch(opcode) {
		default:
			return false;
		case X86_XCHG8rm:
		case X86_XCHG16rm:
		case X86_XCHG32rm:
		case X86_XCHG64rm:
				 return true;
	}
}

// given MCInst's id, find out if this insn is valid for REP prefix
static bool valid_rep(cs_struct *h, unsigned int opcode)
{
	unsigned int id;
	int i = insn_find(insns, ARR_SIZE(insns), opcode, &h->insn_cache);
	if (i != 0) {
		id = insns[i].mapid;
		switch(id) {
			default:
				return false;

			case X86_INS_MOVSB:
			case X86_INS_MOVSW:
			case X86_INS_MOVSQ:

			case X86_INS_LODSB:
			case X86_INS_LODSW:
			case X86_INS_LODSQ:

			case X86_INS_STOSB:
			case X86_INS_STOSW:
			case X86_INS_STOSQ:

			case X86_INS_INSB:
			case X86_INS_INSW:
			case X86_INS_INSD:

			case X86_INS_OUTSB:
			case X86_INS_OUTSW:
			case X86_INS_OUTSD:
				return true;

			// following are some confused instructions, which have the same
			// mnemonics in 128bit media instructions. Intel is horribly crazy!
			case X86_INS_MOVSD:
				if (opcode == X86_MOVSL) // REP MOVSD
					return true;
				return false;

			case X86_INS_LODSD:
				if (opcode == X86_LODSL) // REP LODSD
					return true;
				return false;

			case X86_INS_STOSD:
				if (opcode == X86_STOSL) // REP STOSD
					return true;
				return false;
		}
	}

	// not found
	return false;
}

// given MCInst's id, find out if this insn is valid for REPE prefix
static bool valid_repe(cs_struct *h, unsigned int opcode)
{
	unsigned int id;
	int i = insn_find(insns, ARR_SIZE(insns), opcode, &h->insn_cache);
	if (i != 0) {
		id = insns[i].mapid;
		switch(id) {
			default:
				return false;

			case X86_INS_CMPSB:
			case X86_INS_CMPSW:
			case X86_INS_CMPSQ:

			case X86_INS_SCASB:
			case X86_INS_SCASW:
			case X86_INS_SCASQ:
				return true;

			// following are some confused instructions, which have the same
			// mnemonics in 128bit media instructions. Intel is horribly crazy!
			case X86_INS_CMPSD:
				if (opcode == X86_CMPSL) // REP CMPSD
					return true;
				return false;

			case X86_INS_SCASD:
				if (opcode == X86_SCASL) // REP SCASD
					return true;
				return false;
		}
	}

	// not found
	return false;
}

#ifndef CAPSTONE_DIET
// add *CX register to regs_read[] & regs_write[]
static void add_cx(MCInst *MI)
{
	if (MI->csh->detail) {
		x86_reg cx;

		if (MI->csh->mode & CS_MODE_16)
			cx = X86_REG_CX;
		else if (MI->csh->mode & CS_MODE_32)
			cx = X86_REG_ECX;
		else	// 64-bit
			cx = X86_REG_RCX;

		MI->flat_insn->detail->regs_read[MI->flat_insn->detail->regs_read_count] = cx;
		MI->flat_insn->detail->regs_read_count++;

		MI->flat_insn->detail->regs_write[MI->flat_insn->detail->regs_write_count] = cx;
		MI->flat_insn->detail->regs_write_count++;
	}
}
#endif

// return true if we patch the mnemonic
bool X86_lockrep(MCInst *MI, SStream *O)
{
	unsigned int opcode;
	bool res = false;

	switch(MI->x86_prefix[0]) {
		default:
			break;
		case 0xf0:
#ifndef CAPSTONE_DIET
			if (MI->xAcquireRelease == 0xf2)
				SStream_concat(O, "xacquire|lock|");
			else if (MI->xAcquireRelease == 0xf3)
				SStream_concat(O, "xrelease|lock|");
			else
				SStream_concat(O, "lock|");
#endif
			break;
		case 0xf2:	// repne
			opcode = MCInst_getOpcode(MI);

#ifndef CAPSTONE_DIET	// only care about memonic in standard (non-diet) mode
			if (xchg_mem(opcode) && MI->xAcquireRelease) {
				SStream_concat(O, "xacquire|");
			} else if (valid_repne(MI->csh, opcode)) {
				SStream_concat(O, "repne|");
				add_cx(MI);
			} else if (valid_bnd(MI->csh, opcode)) {
				SStream_concat(O, "bnd|");
			} else {
				// invalid prefix
				MI->x86_prefix[0] = 0;

				// handle special cases
#ifndef CAPSTONE_X86_REDUCE
				if (opcode == X86_MULPDrr) {
					MCInst_setOpcode(MI, X86_MULSDrr);
					SStream_concat(O, "mulsd\t");
					res = true;
				}
#endif
			}
#else	// diet mode -> only patch opcode in special cases
			if (!valid_repne(MI->csh, opcode)) {
				MI->x86_prefix[0] = 0;
			}
#ifndef CAPSTONE_X86_REDUCE
			// handle special cases
			if (opcode == X86_MULPDrr) {
				MCInst_setOpcode(MI, X86_MULSDrr);
			}
#endif
#endif
			break;

		case 0xf3:
			opcode = MCInst_getOpcode(MI);

#ifndef CAPSTONE_DIET	// only care about memonic in standard (non-diet) mode
			if (xchg_mem(opcode) && MI->xAcquireRelease) {
				SStream_concat(O, "xrelease|");
			} else if (valid_rep(MI->csh, opcode)) {
				SStream_concat(O, "rep|");
				add_cx(MI);
			} else if (valid_repe(MI->csh, opcode)) {
				SStream_concat(O, "repe|");
				add_cx(MI);
			} else {
				// invalid prefix
				MI->x86_prefix[0] = 0;

				// handle special cases
#ifndef CAPSTONE_X86_REDUCE
				if (opcode == X86_MULPDrr) {
					MCInst_setOpcode(MI, X86_MULSSrr);
					SStream_concat(O, "mulss\t");
					res = true;
				}
#endif
			}
#else	// diet mode -> only patch opcode in special cases
			if (!valid_rep(MI->csh, opcode) && !valid_repe(MI->csh, opcode)) {
				MI->x86_prefix[0] = 0;
			}
#ifndef CAPSTONE_X86_REDUCE
			// handle special cases
			if (opcode == X86_MULPDrr) {
				MCInst_setOpcode(MI, X86_MULSSrr);
			}
#endif
#endif
			break;
	}

	// copy normalized prefix[] back to x86.prefix[]
	if (MI->csh->detail)
		memcpy(MI->flat_insn->detail->x86.prefix, MI->x86_prefix, ARR_SIZE(MI->x86_prefix));

	return res;
}

void op_addReg(MCInst *MI, int reg)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_REG;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].reg = reg;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->csh->regsize_map[reg];
		MI->flat_insn->detail->x86.op_count++;
	}

	if (MI->op1_size == 0)
		MI->op1_size = MI->csh->regsize_map[reg];
}

void op_addImm(MCInst *MI, int v)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_IMM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].imm = v;
		// if op_count > 0, then this operand's size is taken from the destination op
		if (MI->csh->syntax != CS_OPT_SYNTAX_ATT) {
			if (MI->flat_insn->detail->x86.op_count > 0)
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->flat_insn->detail->x86.operands[0].size;
			else
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->imm_size;
		} else
			MI->has_imm = true;
		MI->flat_insn->detail->x86.op_count++;
	}

	if (MI->op1_size == 0)
		MI->op1_size = MI->imm_size;
}

void op_addXopCC(MCInst *MI, int v)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.xop_cc = v;
	}
}

void op_addSseCC(MCInst *MI, int v)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.sse_cc = v;
	}
}

void op_addAvxCC(MCInst *MI, int v)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.avx_cc = v;
	}
}

void op_addAvxRoundingMode(MCInst *MI, int v)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.avx_rm = v;
	}
}

// below functions supply details to X86GenAsmWriter*.inc
void op_addAvxZeroOpmask(MCInst *MI)
{
	if (MI->csh->detail) {
		// link with the previous operand
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count - 1].avx_zero_opmask = true;
	}
}

void op_addAvxSae(MCInst *MI)
{
	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.avx_sae = true;
	}
}

void op_addAvxBroadcast(MCInst *MI, x86_avx_bcast v)
{
	if (MI->csh->detail) {
		// link with the previous operand
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count - 1].avx_bcast = v;
	}
}

#ifndef CAPSTONE_DIET
// map instruction to its characteristics
typedef struct insn_op {
	uint64_t flags;	// how this instruction update EFLAGS(arithmetic instrcutions) of FPU FLAGS(for FPU instructions)
	uint8_t access[6];
} insn_op;

static insn_op insn_ops[] = {
	{	/* NULL item  */
		0,
		{ 0 }
	},

#ifdef CAPSTONE_X86_REDUCE
#include "X86MappingInsnOp_reduce.inc"
#else
#include "X86MappingInsnOp.inc"
#endif
};

// given internal insn id, return operand access info
uint8_t *X86_get_op_access(cs_struct *h, unsigned int id, uint64_t *eflags)
{
	int i = insn_find(insns, ARR_SIZE(insns), id, &h->insn_cache);
	if (i != 0) {
		*eflags = insn_ops[i].flags;
		return insn_ops[i].access;
	}

	return NULL;
}

void X86_reg_access(const cs_insn *insn,
		cs_regs regs_read, uint8_t *regs_read_count,
		cs_regs regs_write, uint8_t *regs_write_count)
{
	uint8_t i;
	uint8_t read_count, write_count;
	cs_x86 *x86 = &(insn->detail->x86);

	read_count = insn->detail->regs_read_count;
	write_count = insn->detail->regs_write_count;

	// implicit registers
	memcpy(regs_read, insn->detail->regs_read, read_count * sizeof(insn->detail->regs_read[0]));
	memcpy(regs_write, insn->detail->regs_write, write_count * sizeof(insn->detail->regs_write[0]));

	// explicit registers
	for (i = 0; i < x86->op_count; i++) {
		cs_x86_op *op = &(x86->operands[i]);
		switch((int)op->type) {
			case X86_OP_REG:
				if ((op->access & CS_AC_READ) && !arr_exist(regs_read, read_count, op->reg)) {
					regs_read[read_count] = op->reg;
					read_count++;
				}
				if ((op->access & CS_AC_WRITE) && !arr_exist(regs_write, write_count, op->reg)) {
					regs_write[write_count] = op->reg;
					write_count++;
				}
				break;
			case X86_OP_MEM:
				// registers appeared in memory references always being read
				if ((op->mem.segment != X86_REG_INVALID)) {
					regs_read[read_count] = op->mem.segment;
					read_count++;
				}
				if ((op->mem.base != X86_REG_INVALID) && !arr_exist(regs_read, read_count, op->mem.base)) {
					regs_read[read_count] = op->mem.base;
					read_count++;
				}
				if ((op->mem.index != X86_REG_INVALID) && !arr_exist(regs_read, read_count, op->mem.index)) {
					regs_read[read_count] = op->mem.index;
					read_count++;
				}
			default:
				break;
		}
	}

	*regs_read_count = read_count;
	*regs_write_count = write_count;
}
#endif

// map immediate size to instruction id
static struct size_id {
	uint8_t		enc_size;
	uint8_t		size;
	uint16_t 	id;
} x86_imm_size[] = {
#include "X86ImmSize.inc"
};

// given the instruction name, return the size of its immediate operand (or 0)
uint8_t X86_immediate_size(unsigned int id, uint8_t *enc_size)
{
#if 0
	// linear searching
	unsigned int i;

	for (i = 0; i < ARR_SIZE(x86_imm_size); i++) {
		if (id == x86_imm_size[i].id) {
			return x86_imm_size[i].size;
		}
	}
#endif

	// binary searching since the IDs are sorted in order
	unsigned int left, right, m;

	left = 0;
	right = ARR_SIZE(x86_imm_size) - 1;

	while(left <= right) {
		m = (left + right) / 2;
		if (id == x86_imm_size[m].id) {
			if (enc_size != NULL)
				*enc_size = x86_imm_size[m].enc_size;

			return x86_imm_size[m].size;
		}

		if (id < x86_imm_size[m].id)
			right = m - 1;
		else
			left = m + 1;
	}

	// not found
	return 0;
}

#endif
