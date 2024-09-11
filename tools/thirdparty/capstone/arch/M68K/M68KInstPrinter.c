/* Capstone Disassembly Engine */
/* M68K Backend by Daniel Collin <daniel@collin.com> 2015-2016 */

#ifdef _MSC_VER
// Disable security warnings for strcat & sprintf
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

//Banned API Usage : strcat / sprintf is a Banned API as listed in dontuse.h for
//security purposes.
#pragma warning(disable:28719)
#endif

#include <stdio.h>	// DEBUG
#include <stdlib.h>
#include <string.h>

#include "M68KInstPrinter.h"

#include "M68KDisassembler.h"

#include "../../cs_priv.h"
#include "../../utils.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCRegisterInfo.h"

#ifndef CAPSTONE_DIET
static const char* s_spacing = " ";

static const char* s_reg_names[] = {
	"invalid",
	"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
	"fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7",
	"pc",
	"sr", "ccr", "sfc", "dfc", "usp", "vbr", "cacr",
	"caar", "msp", "isp", "tc", "itt0", "itt1", "dtt0",
	"dtt1", "mmusr", "urp", "srp",

	"fpcr", "fpsr", "fpiar",
};

static const char* s_instruction_names[] = {
	"invalid",
	"abcd", "add", "adda", "addi", "addq", "addx", "and", "andi", "asl", "asr", "bhs", "blo", "bhi", "bls", "bcc", "bcs", "bne", "beq", "bvc",
	"bvs", "bpl", "bmi", "bge", "blt", "bgt", "ble", "bra", "bsr", "bchg", "bclr", "bset", "btst", "bfchg", "bfclr", "bfexts", "bfextu", "bfffo", "bfins",
	"bfset", "bftst", "bkpt", "callm", "cas", "cas2", "chk", "chk2", "clr", "cmp", "cmpa", "cmpi", "cmpm", "cmp2", "cinvl", "cinvp", "cinva", "cpushl", "cpushp",
	"cpusha", "dbt", "dbf", "dbhi", "dbls", "dbcc", "dbcs", "dbne", "dbeq", "dbvc", "dbvs", "dbpl", "dbmi", "dbge", "dblt", "dbgt", "dble", "dbra",
	"divs", "divsl", "divu", "divul", "eor", "eori", "exg", "ext", "extb", "fabs", "fsabs", "fdabs", "facos", "fadd", "fsadd", "fdadd", "fasin",
	"fatan", "fatanh", "fbf", "fbeq", "fbogt", "fboge", "fbolt", "fbole", "fbogl", "fbor", "fbun", "fbueq", "fbugt", "fbuge", "fbult", "fbule", "fbne", "fbt",
	"fbsf", "fbseq", "fbgt", "fbge", "fblt", "fble", "fbgl", "fbgle", "fbngle", "fbngl", "fbnle", "fbnlt", "fbnge", "fbngt", "fbsne", "fbst", "fcmp", "fcos",
	"fcosh", "fdbf", "fdbeq", "fdbogt", "fdboge", "fdbolt", "fdbole", "fdbogl", "fdbor", "fdbun", "fdbueq", "fdbugt", "fdbuge", "fdbult", "fdbule", "fdbne",
	"fdbt", "fdbsf", "fdbseq", "fdbgt", "fdbge", "fdblt", "fdble", "fdbgl", "fdbgle", "fdbngle", "fdbngl", "fdbnle", "fdbnlt", "fdbnge", "fdbngt", "fdbsne",
	"fdbst", "fdiv", "fsdiv", "fddiv", "fetox", "fetoxm1", "fgetexp", "fgetman", "fint", "fintrz", "flog10", "flog2", "flogn", "flognp1", "fmod", "fmove",
	"fsmove", "fdmove", "fmovecr", "fmovem", "fmul", "fsmul", "fdmul", "fneg", "fsneg", "fdneg", "fnop", "frem", "frestore", "fsave", "fscale", "fsgldiv",
	"fsglmul", "fsin", "fsincos", "fsinh", "fsqrt", "fssqrt", "fdsqrt", "fsf", "fseq", "fsogt", "fsoge", "fsolt", "fsole", "fsogl", "fsor", "fsun", "fsueq",
	"fsugt", "fsuge", "fsult", "fsule", "fsne", "fst", "fssf", "fsseq", "fsgt", "fsge", "fslt", "fsle", "fsgl", "fsgle", "fsngle",
	"fsngl", "fsnle", "fsnlt", "fsnge", "fsngt", "fssne", "fsst", "fsub", "fssub", "fdsub", "ftan", "ftanh", "ftentox", "ftrapf", "ftrapeq", "ftrapogt",
	"ftrapoge", "ftrapolt", "ftrapole", "ftrapogl", "ftrapor", "ftrapun", "ftrapueq", "ftrapugt", "ftrapuge", "ftrapult", "ftrapule", "ftrapne", "ftrapt",
	"ftrapsf", "ftrapseq", "ftrapgt", "ftrapge", "ftraplt", "ftraple", "ftrapgl", "ftrapgle", "ftrapngle", "ftrapngl", "ftrapnle", "ftrapnlt", "ftrapnge",
	"ftrapngt", "ftrapsne", "ftrapst", "ftst", "ftwotox", "halt", "illegal", "jmp", "jsr", "lea", "link", "lpstop", "lsl", "lsr", "move", "movea", "movec",
	"movem", "movep", "moveq", "moves", "move16", "muls", "mulu", "nbcd", "neg", "negx", "nop", "not", "or", "ori", "pack", "pea", "pflush", "pflusha",
	"pflushan", "pflushn", "ploadr", "ploadw", "plpar", "plpaw", "pmove", "pmovefd", "ptestr", "ptestw", "pulse", "rems", "remu", "reset", "rol", "ror",
	"roxl", "roxr", "rtd", "rte", "rtm", "rtr", "rts", "sbcd", "st", "sf", "shi", "sls", "scc", "shs", "scs", "slo", "sne", "seq", "svc", "svs", "spl", "smi",
	"sge", "slt", "sgt", "sle", "stop", "sub", "suba", "subi", "subq", "subx", "swap", "tas", "trap", "trapv", "trapt", "trapf", "traphi", "trapls",
	"trapcc", "traphs", "trapcs", "traplo", "trapne", "trapeq", "trapvc", "trapvs", "trappl", "trapmi", "trapge", "traplt", "trapgt", "traple", "tst", "unlk", "unpk",
};
#endif


#ifndef CAPSTONE_DIET
static const char* getRegName(m68k_reg reg)
{
	return s_reg_names[(int)reg];
}

static void printRegbitsRange(char* buffer, uint32_t data, const char* prefix)
{
	unsigned int first = 0;
	unsigned int run_length = 0;
	int i;

	for (i = 0; i < 8; ++i) {
		if (data & (1 << i)) {
			first = i;
			run_length = 0;

			while (i < 7 && (data & (1 << (i + 1)))) {
				i++;
				run_length++;
			}

			if (buffer[0] != 0)
				strcat(buffer, "/");

			sprintf(buffer + strlen(buffer), "%s%d", prefix, first);
			if (run_length > 0)
				sprintf(buffer + strlen(buffer), "-%s%d", prefix, first + run_length);
		}
	}
}

static void registerBits(SStream* O, const cs_m68k_op* op)
{
	char buffer[128];
	unsigned int data = op->register_bits;

	buffer[0] = 0;

	if (!data) {
		SStream_concat(O, "%s", "#$0");
		return;
	}

	printRegbitsRange(buffer, data & 0xff, "d");
	printRegbitsRange(buffer, (data >> 8) & 0xff, "a");
	printRegbitsRange(buffer, (data >> 16) & 0xff, "fp");

	SStream_concat(O, "%s", buffer);
}

static void registerPair(SStream* O, const cs_m68k_op* op)
{
	SStream_concat(O, "%s:%s", s_reg_names[op->reg_pair.reg_0],
			s_reg_names[op->reg_pair.reg_1]);
}

static void printAddressingMode(SStream* O, unsigned int pc, const cs_m68k* inst, const cs_m68k_op* op)
{
	switch (op->address_mode) {
		case M68K_AM_NONE:
			switch (op->type) {
				case M68K_OP_REG_BITS:
					registerBits(O, op);
					break;
				case M68K_OP_REG_PAIR:
					registerPair(O, op);
					break;
				case M68K_OP_REG:
					SStream_concat(O, "%s", s_reg_names[op->reg]);
					break;
				default:
					break;
			}
			break;

		case M68K_AM_REG_DIRECT_DATA: SStream_concat(O, "d%d", (op->reg - M68K_REG_D0)); break;
		case M68K_AM_REG_DIRECT_ADDR: SStream_concat(O, "a%d", (op->reg - M68K_REG_A0)); break;
		case M68K_AM_REGI_ADDR: SStream_concat(O, "(a%d)", (op->reg - M68K_REG_A0)); break;
		case M68K_AM_REGI_ADDR_POST_INC: SStream_concat(O, "(a%d)+", (op->reg - M68K_REG_A0)); break;
		case M68K_AM_REGI_ADDR_PRE_DEC: SStream_concat(O, "-(a%d)", (op->reg - M68K_REG_A0)); break;
		case M68K_AM_REGI_ADDR_DISP: SStream_concat(O, "%s$%x(a%d)", op->mem.disp < 0 ? "-" : "", abs(op->mem.disp), (op->mem.base_reg - M68K_REG_A0)); break;
		case M68K_AM_PCI_DISP: SStream_concat(O, "$%x(pc)", pc + 2 + op->mem.disp); break;
		case M68K_AM_ABSOLUTE_DATA_SHORT: SStream_concat(O, "$%x.w", op->imm); break;
		case M68K_AM_ABSOLUTE_DATA_LONG: SStream_concat(O, "$%x.l", op->imm); break;
		case M68K_AM_IMMEDIATE:
			 if (inst->op_size.type == M68K_SIZE_TYPE_FPU) {
#if defined(_KERNEL_MODE)
				 // Issue #681: Windows kernel does not support formatting float point
				 SStream_concat(O, "#<float_point_unsupported>");
				 break;
#else
				 if (inst->op_size.fpu_size == M68K_FPU_SIZE_SINGLE)
					 SStream_concat(O, "#%f", op->simm);
				 else if (inst->op_size.fpu_size == M68K_FPU_SIZE_DOUBLE)
					 SStream_concat(O, "#%f", op->dimm);
				 else
					 SStream_concat(O, "#<unsupported>");
				 break;
#endif
			 }
			 SStream_concat(O, "#$%x", op->imm);
			 break;
		case M68K_AM_PCI_INDEX_8_BIT_DISP:
			SStream_concat(O, "$%x(pc,%s%s.%c)", pc + 2 + op->mem.disp, s_spacing, getRegName(op->mem.index_reg), op->mem.index_size ? 'l' : 'w');
			break;
		case M68K_AM_AREGI_INDEX_8_BIT_DISP:
			SStream_concat(O, "%s$%x(%s,%s%s.%c)", op->mem.disp < 0 ? "-" : "", abs(op->mem.disp), getRegName(op->mem.base_reg), s_spacing, getRegName(op->mem.index_reg), op->mem.index_size ? 'l' : 'w');
			break;
		case M68K_AM_PCI_INDEX_BASE_DISP:
		case M68K_AM_AREGI_INDEX_BASE_DISP:

			if (op->address_mode == M68K_AM_PCI_INDEX_BASE_DISP) {
				SStream_concat(O, "$%x", pc + 2 + op->mem.in_disp);
			} else {
				if (op->mem.in_disp > 0)
					SStream_concat(O, "$%x", op->mem.in_disp);
			}

			SStream_concat(O, "(");

			if (op->address_mode == M68K_AM_PCI_INDEX_BASE_DISP) {
			    SStream_concat(O, "pc,%s.%c", getRegName(op->mem.index_reg), op->mem.index_size ? 'l' : 'w');
			} else {
				if (op->mem.base_reg != M68K_REG_INVALID)
					SStream_concat(O, "a%d,%s", op->mem.base_reg - M68K_REG_A0, s_spacing);
				SStream_concat(O, "%s.%c", getRegName(op->mem.index_reg), op->mem.index_size ? 'l' : 'w');
			}

			if (op->mem.scale > 0)
			    SStream_concat(O, "%s*%s%d)", s_spacing, s_spacing, op->mem.scale);
			else
			    SStream_concat(O, ")");
			break;
			// It's ok to just use PCMI here as is as we set base_reg to PC in the disassembler. While this is not strictly correct it makes the code
			// easier and that is what actually happens when the code is executed anyway.

		case M68K_AM_PC_MEMI_POST_INDEX:
		case M68K_AM_PC_MEMI_PRE_INDEX:
		case M68K_AM_MEMI_PRE_INDEX:
		case M68K_AM_MEMI_POST_INDEX:
			SStream_concat(O, "([");

			if (op->address_mode == M68K_AM_PC_MEMI_POST_INDEX || op->address_mode == M68K_AM_PC_MEMI_PRE_INDEX) {
				SStream_concat(O, "$%x", pc + 2 + op->mem.in_disp);
			} else {
				if (op->mem.in_disp > 0)
					SStream_concat(O, "$%x", op->mem.in_disp);
			}

			if (op->mem.base_reg != M68K_REG_INVALID) {
				if (op->mem.in_disp > 0)
					SStream_concat(O, ",%s%s", s_spacing, getRegName(op->mem.base_reg));
				else
					SStream_concat(O, "%s", getRegName(op->mem.base_reg));
			}

			if (op->address_mode == M68K_AM_MEMI_POST_INDEX || op->address_mode == M68K_AM_PC_MEMI_POST_INDEX)
			    SStream_concat(O, "]");

			if (op->mem.index_reg != M68K_REG_INVALID)
			    SStream_concat(O, ",%s%s.%c", s_spacing, getRegName(op->mem.index_reg), op->mem.index_size ? 'l' : 'w');

			if (op->mem.scale > 0)
			    SStream_concat(O, "%s*%s%d", s_spacing, s_spacing, op->mem.scale);

			if (op->address_mode == M68K_AM_MEMI_PRE_INDEX || op->address_mode == M68K_AM_PC_MEMI_PRE_INDEX)
			    SStream_concat(O, "]");

			if (op->mem.out_disp > 0)
			    SStream_concat(O, ",%s$%x", s_spacing, op->mem.out_disp);

			SStream_concat(O, ")");
			break;
		case M68K_AM_BRANCH_DISPLACEMENT:
			SStream_concat(O, "$%x", pc + 2 + op->br_disp.disp);
		default:
			break;
	}

	if (op->mem.bitfield)
		SStream_concat(O, "{%d:%d}", op->mem.offset, op->mem.width);
}
#endif

#define m68k_sizeof_array(array) (int)(sizeof(array)/sizeof(array[0]))
#define m68k_min(a, b) (a < b) ? a : b

void M68K_printInst(MCInst* MI, SStream* O, void* PrinterInfo)
{
#ifndef CAPSTONE_DIET
	m68k_info *info = (m68k_info *)PrinterInfo;
	cs_m68k *ext = &info->extension;
	cs_detail *detail = NULL;
	int i = 0;

	detail = MI->flat_insn->detail;
	if (detail) {
		int regs_read_count = m68k_min(m68k_sizeof_array(detail->regs_read), info->regs_read_count);
		int regs_write_count = m68k_min(m68k_sizeof_array(detail->regs_write), info->regs_write_count);
		int groups_count = m68k_min(m68k_sizeof_array(detail->groups), info->groups_count);

		memcpy(&detail->m68k, ext, sizeof(cs_m68k));

		memcpy(&detail->regs_read, &info->regs_read, regs_read_count * sizeof(uint16_t));
		detail->regs_read_count = regs_read_count;

		memcpy(&detail->regs_write, &info->regs_write, regs_write_count * sizeof(uint16_t));
		detail->regs_write_count = regs_write_count;

		memcpy(&detail->groups, &info->groups, groups_count);
		detail->groups_count = groups_count;
	}

	if (MI->Opcode == M68K_INS_INVALID) {
		if (ext->op_count)
			SStream_concat(O, "dc.w $%x", ext->operands[0].imm);
		else
			SStream_concat(O, "dc.w $<unknown>");
		return;
	}

	SStream_concat0(O, (char*)s_instruction_names[MI->Opcode]);

	switch (ext->op_size.type) {
		case M68K_SIZE_TYPE_INVALID :
			break;

		case M68K_SIZE_TYPE_CPU :
			switch (ext->op_size.cpu_size) {
				case M68K_CPU_SIZE_BYTE: SStream_concat0(O, ".b"); break;
				case M68K_CPU_SIZE_WORD: SStream_concat0(O, ".w"); break;
				case M68K_CPU_SIZE_LONG: SStream_concat0(O, ".l"); break;
				case M68K_CPU_SIZE_NONE: break;
			}
			break;

		case M68K_SIZE_TYPE_FPU :
			switch (ext->op_size.fpu_size) {
				case M68K_FPU_SIZE_SINGLE: SStream_concat0(O, ".s"); break;
				case M68K_FPU_SIZE_DOUBLE: SStream_concat0(O, ".d"); break;
				case M68K_FPU_SIZE_EXTENDED: SStream_concat0(O, ".x"); break;
				case M68K_FPU_SIZE_NONE: break;
			}
			break;
	}

	SStream_concat0(O, " ");

	// this one is a bit spacial so we do special things

	if (MI->Opcode == M68K_INS_CAS2) {
		int reg_value_0, reg_value_1;
		printAddressingMode(O, info->pc, ext, &ext->operands[0]); SStream_concat0(O, ",");
		printAddressingMode(O, info->pc, ext, &ext->operands[1]); SStream_concat0(O, ",");
		reg_value_0 = ext->operands[2].register_bits >> 4;
		reg_value_1 = ext->operands[2].register_bits & 0xf;
		SStream_concat(O, "(%s):(%s)", s_reg_names[M68K_REG_D0 + reg_value_0], s_reg_names[M68K_REG_D0 + reg_value_1]);
		return;
	}

	for (i  = 0; i < ext->op_count; ++i) {
		printAddressingMode(O, info->pc, ext, &ext->operands[i]);
		if ((i + 1) != ext->op_count)
			SStream_concat(O, ",%s", s_spacing);
	}
#endif
}

const char* M68K_reg_name(csh handle, unsigned int reg)
{
#ifdef CAPSTONE_DIET
	return NULL;
#else
	if (reg >= ARR_SIZE(s_reg_names)) {
		return NULL;
	}
	return s_reg_names[(int)reg];
#endif
}

void M68K_get_insn_id(cs_struct* h, cs_insn* insn, unsigned int id)
{
	insn->id = id; // These id's matches for 68k
}

const char* M68K_insn_name(csh handle, unsigned int id)
{
#ifdef CAPSTONE_DIET
	return NULL;
#else
	return s_instruction_names[id];
#endif
}

#ifndef CAPSTONE_DIET
static name_map group_name_maps[] = {
	{ M68K_GRP_INVALID , NULL },
	{ M68K_GRP_JUMP, "jump" },
	{ M68K_GRP_RET , "ret" },
	{ M68K_GRP_IRET, "iret" },
	{ M68K_GRP_BRANCH_RELATIVE, "branch_relative" },
};
#endif

const char *M68K_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(group_name_maps, ARR_SIZE(group_name_maps), id);
#else
	return NULL;
#endif
}

