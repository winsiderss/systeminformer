/* Capstone Disassembly Engine */
/* TMS320C64x Backend by Fotis Loukos <me@fotisl.com> 2016 */

#ifdef CAPSTONE_HAS_TMS320C64X

#ifdef _MSC_VER
// Disable security warnings for strcpy
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// Banned API Usage : strcpy is a Banned API as listed in dontuse.h for
// security purposes.
#pragma warning(disable:28719)
#endif

#include <ctype.h>
#include <string.h>

#include "TMS320C64xInstPrinter.h"
#include "../../MCInst.h"
#include "../../utils.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "../../MathExtras.h"
#include "TMS320C64xMapping.h"

#include "capstone/tms320c64x.h"

static char *getRegisterName(unsigned RegNo);
static void printOperand(MCInst *MI, unsigned OpNo, SStream *O);
static void printMemOperand(MCInst *MI, unsigned OpNo, SStream *O);
static void printMemOperand2(MCInst *MI, unsigned OpNo, SStream *O);
static void printRegPair(MCInst *MI, unsigned OpNo, SStream *O);

void TMS320C64x_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci)
{
	SStream ss;
	char *p, *p2, tmp[8];
	unsigned int unit = 0;
	int i;
	cs_tms320c64x *tms320c64x;

	if (mci->csh->detail) {
		tms320c64x = &mci->flat_insn->detail->tms320c64x;

		for (i = 0; i < insn->detail->groups_count; i++) {
			switch(insn->detail->groups[i]) {
				case TMS320C64X_GRP_FUNIT_D:
					unit = TMS320C64X_FUNIT_D;
					break;
				case TMS320C64X_GRP_FUNIT_L:
					unit = TMS320C64X_FUNIT_L;
					break;
				case TMS320C64X_GRP_FUNIT_M:
					unit = TMS320C64X_FUNIT_M;
					break;
				case TMS320C64X_GRP_FUNIT_S:
					unit = TMS320C64X_FUNIT_S;
					break;
				case TMS320C64X_GRP_FUNIT_NO:
					unit = TMS320C64X_FUNIT_NO;
					break;
			}
			if (unit != 0)
				break;
		}
		tms320c64x->funit.unit = unit;

		SStream_Init(&ss);
		if (tms320c64x->condition.reg != TMS320C64X_REG_INVALID)
			SStream_concat(&ss, "[%c%s]|", (tms320c64x->condition.zero == 1) ? '!' : '|', cs_reg_name(ud, tms320c64x->condition.reg));

		p = strchr(insn_asm, '\t');
		if (p != NULL)
			*p++ = '\0';

		SStream_concat0(&ss, insn_asm);
		if ((p != NULL) && (((p2 = strchr(p, '[')) != NULL) || ((p2 = strchr(p, '(')) != NULL))) {
			while ((p2 > p) && ((*p2 != 'a') && (*p2 != 'b')))
				p2--;
			if (p2 == p) {
				strcpy(insn_asm, "Invalid!");
				return;
			}
			if (*p2 == 'a')
				strcpy(tmp, "1T");
			else
				strcpy(tmp, "2T");
		} else {
			tmp[0] = '\0';
		}
		switch(tms320c64x->funit.unit) {
			case TMS320C64X_FUNIT_D:
				SStream_concat(&ss, ".D%s%u", tmp, tms320c64x->funit.side);
				break;
			case TMS320C64X_FUNIT_L:
				SStream_concat(&ss, ".L%s%u", tmp, tms320c64x->funit.side);
				break;
			case TMS320C64X_FUNIT_M:
				SStream_concat(&ss, ".M%s%u", tmp, tms320c64x->funit.side);
				break;
			case TMS320C64X_FUNIT_S:
				SStream_concat(&ss, ".S%s%u", tmp, tms320c64x->funit.side);
				break;
		}
		if (tms320c64x->funit.crosspath > 0)
			SStream_concat0(&ss, "X");

		if (p != NULL)
			SStream_concat(&ss, "\t%s", p);

		if (tms320c64x->parallel != 0)
			SStream_concat(&ss, "\t||");

		/* insn_asm is a buffer from an SStream, so there should be enough space */
		strcpy(insn_asm, ss.buffer);
	}
}

#define PRINT_ALIAS_INSTR
#include "TMS320C64xGenAsmWriter.inc"

#define GET_INSTRINFO_ENUM
#include "TMS320C64xGenInstrInfo.inc"

static void printOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	unsigned reg;

	if (MCOperand_isReg(Op)) {
		reg = MCOperand_getReg(Op);
		if ((MCInst_getOpcode(MI) == TMS320C64x_MVC_s1_rr) && (OpNo == 1)) {
			switch(reg) {
				case TMS320C64X_REG_EFR:
					SStream_concat0(O, "EFR");
					break;
				case TMS320C64X_REG_IFR:
					SStream_concat0(O, "IFR");
					break;
				default:
					SStream_concat0(O, getRegisterName(reg));
					break;
			}
		} else {
			SStream_concat0(O, getRegisterName(reg));
		}

		if (MI->csh->detail) {
			MI->flat_insn->detail->tms320c64x.operands[MI->flat_insn->detail->tms320c64x.op_count].type = TMS320C64X_OP_REG;
			MI->flat_insn->detail->tms320c64x.operands[MI->flat_insn->detail->tms320c64x.op_count].reg = reg;
			MI->flat_insn->detail->tms320c64x.op_count++;
		}
	} else if (MCOperand_isImm(Op)) {
		int64_t Imm = MCOperand_getImm(Op);

		if (Imm >= 0) {
			if (Imm > HEX_THRESHOLD)
				SStream_concat(O, "0x%"PRIx64, Imm);
			else
				SStream_concat(O, "%"PRIu64, Imm);
		} else {
			if (Imm < -HEX_THRESHOLD)
				SStream_concat(O, "-0x%"PRIx64, -Imm);
			else
				SStream_concat(O, "-%"PRIu64, -Imm);
		}

		if (MI->csh->detail) {
			MI->flat_insn->detail->tms320c64x.operands[MI->flat_insn->detail->tms320c64x.op_count].type = TMS320C64X_OP_IMM;
			MI->flat_insn->detail->tms320c64x.operands[MI->flat_insn->detail->tms320c64x.op_count].imm = Imm;
			MI->flat_insn->detail->tms320c64x.op_count++;
		}
	}
}

static void printMemOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	int64_t Val = MCOperand_getImm(Op);
	unsigned scaled, base, offset, mode, unit;
	cs_tms320c64x *tms320c64x;
	char st, nd;

	scaled = (Val >> 19) & 1;
	base = (Val >> 12) & 0x7f;
	offset = (Val >> 5) & 0x7f;
	mode = (Val >> 1) & 0xf;
	unit = Val & 1;

	if (scaled) {
		st = '[';
		nd = ']';
	} else {
		st = '(';
		nd = ')';
	}

	switch(mode) {
		case 0:
			SStream_concat(O, "*-%s%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 1:
			SStream_concat(O, "*+%s%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 4:
			SStream_concat(O, "*-%s%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
		case 5:
			SStream_concat(O, "*+%s%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
		case 8:
			SStream_concat(O, "*--%s%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 9:
			SStream_concat(O, "*++%s%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 10:
			SStream_concat(O, "*%s--%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 11:
			SStream_concat(O, "*%s++%c%u%c", getRegisterName(base), st, offset, nd);
			break;
		case 12:
			SStream_concat(O, "*--%s%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
		case 13:
			SStream_concat(O, "*++%s%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
		case 14:
			SStream_concat(O, "*%s--%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
		case 15:
			SStream_concat(O, "*%s++%c%s%c", getRegisterName(base), st, getRegisterName(offset), nd);
			break;
	}

	if (MI->csh->detail) {
		tms320c64x = &MI->flat_insn->detail->tms320c64x;

		tms320c64x->operands[tms320c64x->op_count].type = TMS320C64X_OP_MEM;
		tms320c64x->operands[tms320c64x->op_count].mem.base = base;
		tms320c64x->operands[tms320c64x->op_count].mem.disp = offset;
		tms320c64x->operands[tms320c64x->op_count].mem.unit = unit + 1;
		tms320c64x->operands[tms320c64x->op_count].mem.scaled = scaled;
		switch(mode) {
			case 0:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_NO;
				break;
			case 1:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_NO;
				break;
			case 4:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_NO;
				break;
			case 5:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_NO;
				break;
			case 8:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_PRE;
				break;
			case 9:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_PRE;
				break;
			case 10:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_POST;
				break;
			case 11:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_POST;
				break;
			case 12:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_PRE;
				break;
			case 13:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_PRE;
				break;
			case 14:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_BW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_POST;
				break;
			case 15:
				tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_REGISTER;
				tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
				tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_POST;
				break;
		}
		tms320c64x->op_count++;
	}
}

static void printMemOperand2(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	int64_t Val = MCOperand_getImm(Op);
	uint16_t offset;
	unsigned basereg;
	cs_tms320c64x *tms320c64x;

	basereg = Val & 0x7f;
	offset = (Val >> 7) & 0x7fff;
	SStream_concat(O, "*+%s[0x%x]", getRegisterName(basereg), offset);

	if (MI->csh->detail) {
		tms320c64x = &MI->flat_insn->detail->tms320c64x;

		tms320c64x->operands[tms320c64x->op_count].type = TMS320C64X_OP_MEM;
		tms320c64x->operands[tms320c64x->op_count].mem.base = basereg;
		tms320c64x->operands[tms320c64x->op_count].mem.unit = 2;
		tms320c64x->operands[tms320c64x->op_count].mem.disp = offset;
		tms320c64x->operands[tms320c64x->op_count].mem.disptype = TMS320C64X_MEM_DISP_CONSTANT;
		tms320c64x->operands[tms320c64x->op_count].mem.direction = TMS320C64X_MEM_DIR_FW;
		tms320c64x->operands[tms320c64x->op_count].mem.modify = TMS320C64X_MEM_MOD_NO;
		tms320c64x->op_count++;
	}
}

static void printRegPair(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	unsigned reg = MCOperand_getReg(Op);
	cs_tms320c64x *tms320c64x;

	SStream_concat(O, "%s:%s", getRegisterName(reg + 1), getRegisterName(reg));

	if (MI->csh->detail) {
		tms320c64x = &MI->flat_insn->detail->tms320c64x;

		tms320c64x->operands[tms320c64x->op_count].type = TMS320C64X_OP_REGPAIR;
		tms320c64x->operands[tms320c64x->op_count].reg = reg;
		tms320c64x->op_count++;
	}
}

static bool printAliasInstruction(MCInst *MI, SStream *O, MCRegisterInfo *MRI)
{
	unsigned opcode = MCInst_getOpcode(MI);
	MCOperand *op;

	switch(opcode) {
		/* ADD.Dx -i, x, y -> SUB.Dx x, i, y */
		case TMS320C64x_ADD_d2_rir:
		/* ADD.L -i, x, y -> SUB.L x, i, y */
		case TMS320C64x_ADD_l1_irr:
		case TMS320C64x_ADD_l1_ipp:
		/* ADD.S -i, x, y -> SUB.S x, i, y */
		case TMS320C64x_ADD_s1_irr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isImm(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getImm(MCInst_getOperand(MI, 2)) < 0)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_SUB);
				op = MCInst_getOperand(MI, 2);
				MCOperand_setImm(op, -MCOperand_getImm(op));

				SStream_concat0(O, "SUB\t");
				printOperand(MI, 1, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 2, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* ADD.D 0, x, y -> MV.D x, y */
		case TMS320C64x_ADD_d1_rir:
		/* OR.D x, 0, y -> MV.D x, y */
		case TMS320C64x_OR_d2_rir:
		/* ADD.L 0, x, y -> MV.L x, y */
		case TMS320C64x_ADD_l1_irr:
		case TMS320C64x_ADD_l1_ipp:
		/* OR.L 0, x, y -> MV.L x, y */
		case TMS320C64x_OR_l1_irr:
		/* ADD.S 0, x, y -> MV.S x, y */
		case TMS320C64x_ADD_s1_irr:
		/* OR.S 0, x, y -> MV.S x, y */
		case TMS320C64x_OR_s1_irr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isImm(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getImm(MCInst_getOperand(MI, 2)) == 0)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_MV);
				MI->size--;

				SStream_concat0(O, "MV\t");
				printOperand(MI, 1, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* XOR.D -1, x, y -> NOT.D x, y */
		case TMS320C64x_XOR_d2_rir:
		/* XOR.L -1, x, y -> NOT.L x, y */
		case TMS320C64x_XOR_l1_irr:
		/* XOR.S -1, x, y -> NOT.S x, y */
		case TMS320C64x_XOR_s1_irr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isImm(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getImm(MCInst_getOperand(MI, 2)) == -1)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_NOT);
				MI->size--;

				SStream_concat0(O, "NOT\t");
				printOperand(MI, 1, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* MVK.D 0, x -> ZERO.D x */
		case TMS320C64x_MVK_d1_rr:
		/* MVK.L 0, x -> ZERO.L x */
		case TMS320C64x_MVK_l2_ir:
			if ((MCInst_getNumOperands(MI) == 2) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isImm(MCInst_getOperand(MI, 1)) &&
				(MCOperand_getImm(MCInst_getOperand(MI, 1)) == 0)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_ZERO);
				MI->size--;

				SStream_concat0(O, "ZERO\t");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* SUB.L x, x, y -> ZERO.L y */
		case TMS320C64x_SUB_l1_rrp_x1:
		/* SUB.S x, x, y -> ZERO.S y */
		case TMS320C64x_SUB_s1_rrr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getReg(MCInst_getOperand(MI, 1)) == MCOperand_getReg(MCInst_getOperand(MI, 2)))) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_ZERO);
				MI->size -= 2;

				SStream_concat0(O, "ZERO\t");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* SUB.L 0, x, y -> NEG.L x, y */
		case TMS320C64x_SUB_l1_irr:
		case TMS320C64x_SUB_l1_ipp:
		/* SUB.S 0, x, y -> NEG.S x, y */
		case TMS320C64x_SUB_s1_irr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isImm(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getImm(MCInst_getOperand(MI, 2)) == 0)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_NEG);
				MI->size--;

				SStream_concat0(O, "NEG\t");
				printOperand(MI, 1, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* PACKLH2.L x, x, y -> SWAP2.L x, y */
		case TMS320C64x_PACKLH2_l1_rrr_x2:
		/* PACKLH2.S x, x, y -> SWAP2.S x, y */
		case TMS320C64x_PACKLH2_s1_rrr:
			if ((MCInst_getNumOperands(MI) == 3) &&
				MCOperand_isReg(MCInst_getOperand(MI, 0)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 1)) &&
				MCOperand_isReg(MCInst_getOperand(MI, 2)) &&
				(MCOperand_getReg(MCInst_getOperand(MI, 1)) == MCOperand_getReg(MCInst_getOperand(MI, 2)))) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_SWAP2);
				MI->size--;

				SStream_concat0(O, "SWAP2\t");
				printOperand(MI, 1, O);
				SStream_concat0(O, ", ");
				printOperand(MI, 0, O);

				return true;
			}
			break;
	}
	switch(opcode) {
		/* NOP 16 -> IDLE */
		/* NOP 1 -> NOP */
		case TMS320C64x_NOP_n:
			if ((MCInst_getNumOperands(MI) == 1) &&
				MCOperand_isImm(MCInst_getOperand(MI, 0)) &&
				(MCOperand_getReg(MCInst_getOperand(MI, 0)) == 16)) {

				MCInst_setOpcodePub(MI, TMS320C64X_INS_IDLE);
				MI->size--;

				SStream_concat0(O, "IDLE");

				return true;
			}
			if ((MCInst_getNumOperands(MI) == 1) &&
				MCOperand_isImm(MCInst_getOperand(MI, 0)) &&
				(MCOperand_getReg(MCInst_getOperand(MI, 0)) == 1)) {

				MI->size--;

				SStream_concat0(O, "NOP");

				return true;
			}
			break;
	}

	return false;
}

void TMS320C64x_printInst(MCInst *MI, SStream *O, void *Info)
{
	if (!printAliasInstruction(MI, O, Info))
		printInstruction(MI, O, Info);
}

#endif
