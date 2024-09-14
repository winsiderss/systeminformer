/* Capstone Disassembly Engine */
/* M680X Backend by Wolfgang Schwotzer <wolfgang.schwotzer@gmx.net> 2017 */

#ifdef CAPSTONE_HAS_M680X
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <capstone/platform.h>

#include "../../cs_priv.h"
#include "../../MCInst.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "../../utils.h"
#include "M680XInstPrinter.h"
#include "M680XDisassembler.h"
#include "M680XDisassemblerInternals.h"

#ifndef CAPSTONE_DIET
static const char s_reg_names[][10] = {
	"<invalid>", "a", "b", "e", "f", "0", "d", "w", "cc", "dp", "md",
	"hx", "h", "x", "y", "s", "u", "v", "q", "pc", "tmp2", "tmp3",
};

static const char s_instruction_names[][6] = {
	"invld", "aba", "abx", "aby", "adc", "adca", "adcb", "adcd", "adcr",
	"add", "adda", "addb", "addd", "adde", "addf", "addr", "addw",
	"aim", "ais", "aix", "and", "anda", "andb", "andcc", "andd", "andr",
	"asl", "asla", "aslb", "asld",
	"asr", "asra", "asrb", "asrd", "asrx",
	"band",
	"bcc", "bclr", "bcs", "beor", "beq", "bge", "bgnd", "bgt", "bhcc",
	"bhcs", "bhi",
	"biand", "bieor", "bih", "bil",
	"bior", "bit", "bita", "bitb", "bitd", "bitmd", "ble", "bls", "blt",
	"bmc",
	"bmi", "bms",
	"bne", "bor", "bpl", "brclr", "brset", "bra", "brn", "bset", "bsr",
	"bvc", "bvs",
	"call", "cba", "cbeq", "cbeqa", "cbeqx", "clc", "cli",
	"clr", "clra", "clrb", "clrd", "clre", "clrf", "clrh", "clrw", "clrx",
	"clv", "cmp",
	"cmpa", "cmpb", "cmpd", "cmpe", "cmpf", "cmpr", "cmps", "cmpu", "cmpw",
	"cmpx", "cmpy",
	"com", "coma", "comb", "comd", "come", "comf", "comw", "comx", "cpd",
	"cphx", "cps", "cpx", "cpy",
	"cwai", "daa", "dbeq", "dbne", "dbnz", "dbnza", "dbnzx",
	"dec", "deca", "decb", "decd", "dece", "decf", "decw",
	"decx", "des", "dex", "dey",
	"div", "divd", "divq", "ediv", "edivs", "eim", "emacs", "emaxd",
	"emaxm", "emind", "eminm", "emul", "emuls",
	"eor", "eora", "eorb", "eord", "eorr", "etbl",
	"exg", "fdiv", "ibeq", "ibne", "idiv", "idivs", "illgl",
	"inc", "inca", "incb", "incd", "ince", "incf", "incw", "incx",
	"ins", "inx", "iny",
	"jmp", "jsr",
	"lbcc", "lbcs", "lbeq", "lbge", "lbgt", "lbhi", "lble", "lbls", "lblt",
	"lbmi", "lbne", "lbpl", "lbra", "lbrn", "lbsr", "lbvc", "lbvs",
	"lda", "ldaa", "ldab", "ldb", "ldbt", "ldd", "lde", "ldf", "ldhx",
	"ldmd",
	"ldq", "lds", "ldu", "ldw", "ldx", "ldy",
	"leas", "leau", "leax", "leay",
	"lsl", "lsla", "lslb", "lsld", "lslx",
	"lsr", "lsra", "lsrb", "lsrd", "lsrw", "lsrx",
	"maxa", "maxm", "mem", "mina", "minm", "mov", "movb", "movw", "mul",
	"muld",
	"neg", "nega", "negb", "negd", "negx",
	"nop", "nsa", "oim", "ora", "oraa", "orab", "orb", "orcc", "ord", "orr",
	"psha", "pshb", "pshc", "pshd", "pshh", "pshs", "pshsw", "pshu",
	"pshuw", "pshx", "pshy",
	"pula", "pulb", "pulc", "puld", "pulh", "puls", "pulsw", "pulu",
	"puluw", "pulx", "puly", "rev", "revw",
	"rol", "rola", "rolb", "rold", "rolw", "rolx",
	"ror", "rora", "rorb", "rord", "rorw", "rorx",
	"rsp", "rtc", "rti", "rts", "sba", "sbc", "sbca", "sbcb", "sbcd",
	"sbcr",
	"sec", "sei", "sev", "sex", "sexw", "slp", "sta", "staa", "stab", "stb",
	"stbt", "std", "ste", "stf", "stop", "sthx",
	"stq", "sts", "stu", "stw", "stx", "sty",
	"sub", "suba", "subb", "subd", "sube", "subf", "subr", "subw",
	"swi", "swi2", "swi3",
	"sync", "tab", "tap", "tax", "tba", "tbeq", "tbl", "tbne", "test",
	"tfm", "tfr",
	"tim", "tpa",
	"tst", "tsta", "tstb", "tstd", "tste", "tstf", "tstw", "tstx",
	"tsx", "tsy", "txa", "txs", "tys", "wai", "wait", "wav", "wavr",
	"xgdx", "xgdy",
};

static name_map s_group_names[] = {
	{ M680X_GRP_INVALID, "<invalid>" },
	{ M680X_GRP_JUMP,  "jump" },
	{ M680X_GRP_CALL,  "call" },
	{ M680X_GRP_RET, "return" },
	{ M680X_GRP_INT, "interrupt" },
	{ M680X_GRP_IRET,  "interrupt_return" },
	{ M680X_GRP_PRIV,  "privileged" },
	{ M680X_GRP_BRAREL,  "branch_relative" },
};
#endif

static void printRegName(cs_struct *handle, SStream *OS, unsigned int reg)
{
#ifndef CAPSTONE_DIET
	SStream_concat(OS, handle->reg_name((csh)handle, reg));
#endif
}

static void printInstructionName(cs_struct *handle, SStream *OS,
	unsigned int insn)
{
#ifndef CAPSTONE_DIET
	SStream_concat(OS, handle->insn_name((csh)handle, insn));
#endif
}

static uint32_t get_unsigned(int32_t value, int byte_size)
{
	switch (byte_size) {
	case 1:
		return (uint32_t)(value & 0xff);

	case 2:
		return (uint32_t)(value & 0xffff);

	default:
	case 4:
		return (uint32_t)value;
	}
}

static void printIncDec(bool isPost, SStream *O, m680x_info *info,
	cs_m680x_op *op)
{
	static const char s_inc_dec[][3] = { "--", "-", "", "+", "++" };

	if (!op->idx.inc_dec)
		return;

	if ((!isPost && !(op->idx.flags & M680X_IDX_POST_INC_DEC)) ||
		(isPost && (op->idx.flags & M680X_IDX_POST_INC_DEC))) {
		const char *prePostfix = "";

		if (info->cpu_type == M680X_CPU_TYPE_CPU12)
			prePostfix = (op->idx.inc_dec < 0) ? "-" : "+";
		else if (op->idx.inc_dec >= -2 && (op->idx.inc_dec <= 2)) {
			prePostfix = (char *)s_inc_dec[op->idx.inc_dec + 2];
		}

		SStream_concat(O, prePostfix);
	}
}

static void printOperand(MCInst *MI, SStream *O, m680x_info *info,
	cs_m680x_op *op)
{
	switch (op->type) {
	case M680X_OP_REGISTER:
		printRegName(MI->csh, O, op->reg);
		break;

	case M680X_OP_CONSTANT:
		SStream_concat(O, "%u", op->const_val);
		break;

	case M680X_OP_IMMEDIATE:
		if (MI->csh->imm_unsigned)
			SStream_concat(O, "#%u",
				get_unsigned(op->imm, op->size));
		else
			SStream_concat(O, "#%d", op->imm);

		break;

	case M680X_OP_INDEXED:
		if (op->idx.flags & M680X_IDX_INDIRECT)
			SStream_concat(O, "[");

		if (op->idx.offset_reg != M680X_REG_INVALID)
			printRegName(MI->csh, O, op->idx.offset_reg);
		else if (op->idx.offset_bits > 0) {
			if (op->idx.base_reg == M680X_REG_PC)
				SStream_concat(O, "$%04x", op->idx.offset_addr);
			else
				SStream_concat(O, "%d", op->idx.offset);
		}
		else if (op->idx.inc_dec != 0 &&
			info->cpu_type == M680X_CPU_TYPE_CPU12)
			SStream_concat(O, "%d", abs(op->idx.inc_dec));

		if (!(op->idx.flags & M680X_IDX_NO_COMMA))
			SStream_concat(O, ", ");

		printIncDec(false, O, info, op);

		printRegName(MI->csh, O, op->idx.base_reg);

		if (op->idx.base_reg == M680X_REG_PC &&
			(op->idx.offset_bits > 0))
			SStream_concat(O, "r");

		printIncDec(true, O, info, op);

		if (op->idx.flags & M680X_IDX_INDIRECT)
			SStream_concat(O, "]");

		break;

	case M680X_OP_RELATIVE:
		SStream_concat(O, "$%04x", op->rel.address);
		break;

	case M680X_OP_DIRECT:
		SStream_concat(O, "$%02x", op->direct_addr);
		break;

	case M680X_OP_EXTENDED:
		if (op->ext.indirect)
			SStream_concat(O, "[$%04x]", op->ext.address);
		else {
			if (op->ext.address < 256) {
				SStream_concat(O, ">$%04x", op->ext.address);
			}
			else {
				SStream_concat(O, "$%04x", op->ext.address);
			}
		}

		break;

	default:
		SStream_concat(O, "<invalid_operand>");
		break;
	}
}

static const char *getDelimiter(m680x_info *info, cs_m680x *m680x)
{
	bool indexed = false;
	int count = 0;
	int i;

	if (info->insn == M680X_INS_TFM)
		return ", ";

	if (m680x->op_count > 1) {
		for (i  = 0; i < m680x->op_count; ++i) {
			if (m680x->operands[i].type == M680X_OP_INDEXED)
				indexed = true;

			if (m680x->operands[i].type != M680X_OP_REGISTER)
				count++;
		}
	}

	return (indexed && (count >= 1)) ? "; " : ", ";
};

void M680X_printInst(MCInst *MI, SStream *O, void *PrinterInfo)
{
	m680x_info *info = (m680x_info *)PrinterInfo;
	cs_m680x *m680x = &info->m680x;
	cs_detail *detail = MI->flat_insn->detail;
	int suppress_operands = 0;
	const char *delimiter = getDelimiter(info, m680x);
	int i;

	if (detail != NULL)
		memcpy(&detail->m680x, m680x, sizeof(cs_m680x));

	if (info->insn == M680X_INS_INVLD || info->insn == M680X_INS_ILLGL) {
		if (m680x->op_count)
			SStream_concat(O, "fcb $%02x", m680x->operands[0].imm);
		else
			SStream_concat(O, "fcb $<unknown>");

		return;
	}

	printInstructionName(MI->csh, O, info->insn);
	SStream_concat(O, " ");

	if ((m680x->flags & M680X_FIRST_OP_IN_MNEM) != 0)
		suppress_operands++;

	if ((m680x->flags & M680X_SECOND_OP_IN_MNEM) != 0)
		suppress_operands++;

	for (i  = 0; i < m680x->op_count; ++i) {
		if (i >= suppress_operands) {
			printOperand(MI, O, info, &m680x->operands[i]);

			if ((i + 1) != m680x->op_count)
				SStream_concat(O, delimiter);
		}
	}
}

const char *M680X_reg_name(csh handle, unsigned int reg)
{
#ifndef CAPSTONE_DIET

	if (reg >= ARR_SIZE(s_reg_names))
		return NULL;

	return s_reg_names[(int)reg];
#else
	return NULL;
#endif
}

const char *M680X_insn_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET

	if (id >= ARR_SIZE(s_instruction_names))
		return NULL;
	else
		return s_instruction_names[(int)id];

#else
	return NULL;
#endif
}

const char *M680X_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(s_group_names, ARR_SIZE(s_group_names), id);
#else
	return NULL;
#endif
}

cs_err M680X_instprinter_init(cs_struct *ud)
{
#ifndef CAPSTONE_DIET

	if (M680X_REG_ENDING != ARR_SIZE(s_reg_names)) {
		CS_ASSERT(M680X_REG_ENDING == ARR_SIZE(s_reg_names));
		return CS_ERR_MODE;
	}

	if (M680X_INS_ENDING != ARR_SIZE(s_instruction_names)) {
		CS_ASSERT(M680X_INS_ENDING == ARR_SIZE(s_instruction_names));
		return CS_ERR_MODE;
	}

	if (M680X_GRP_ENDING != ARR_SIZE(s_group_names)) {
		CS_ASSERT(M680X_GRP_ENDING == ARR_SIZE(s_group_names));
		return CS_ERR_MODE;
	}

#endif

	return CS_ERR_OK;
}

#endif

