/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_XCORE

#include <stdio.h>	// debug
#include <string.h>

#include "../../utils.h"

#include "XCoreMapping.h"

#define GET_INSTRINFO_ENUM
#include "XCoreGenInstrInfo.inc"

static const name_map reg_name_maps[] = {
	{ XCORE_REG_INVALID, NULL },

	{ XCORE_REG_CP, "cp" },
	{ XCORE_REG_DP, "dp" },
	{ XCORE_REG_LR, "lr" },
	{ XCORE_REG_SP, "sp" },
	{ XCORE_REG_R0, "r0" },
	{ XCORE_REG_R1, "r1" },
	{ XCORE_REG_R2, "r2" },
	{ XCORE_REG_R3, "r3" },
	{ XCORE_REG_R4, "r4" },
	{ XCORE_REG_R5, "r5" },
	{ XCORE_REG_R6, "r6" },
	{ XCORE_REG_R7, "r7" },
	{ XCORE_REG_R8, "r8" },
	{ XCORE_REG_R9, "r9" },
	{ XCORE_REG_R10, "r10" },
	{ XCORE_REG_R11, "r11" },

	// pseudo registers
	{ XCORE_REG_PC, "pc" },

	{ XCORE_REG_SCP, "scp" },
	{ XCORE_REG_SSR, "ssr" },
	{ XCORE_REG_ET, "et" },
	{ XCORE_REG_ED, "ed" },
	{ XCORE_REG_SED, "sed" },
	{ XCORE_REG_KEP, "kep" },
	{ XCORE_REG_KSP, "ksp" },
	{ XCORE_REG_ID, "id" },
};

const char *XCore_reg_name(csh handle, unsigned int reg)
{
#ifndef CAPSTONE_DIET
	if (reg >= ARR_SIZE(reg_name_maps))
		return NULL;

	return reg_name_maps[reg].name;
#else
	return NULL;
#endif
}

xcore_reg XCore_reg_id(char *name)
{
	int i;

	for(i = 1; i < ARR_SIZE(reg_name_maps); i++) {
		if (!strcmp(name, reg_name_maps[i].name))
			return reg_name_maps[i].id;
	}

	// not found
	return 0;
}

static const insn_map insns[] = {
	// dummy item
	{
		0, 0,
#ifndef CAPSTONE_DIET
		{ 0 }, { 0 }, { 0 }, 0, 0
#endif
	},

#include "XCoreMappingInsn.inc"
};

// given internal insn id, return public instruction info
void XCore_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id)
{
	unsigned short i;

	i = insn_find(insns, ARR_SIZE(insns), id, &h->insn_cache);
	if (i != 0) {
		insn->id = insns[i].mapid;

		if (h->detail) {
#ifndef CAPSTONE_DIET
			memcpy(insn->detail->regs_read, insns[i].regs_use, sizeof(insns[i].regs_use));
			insn->detail->regs_read_count = (uint8_t)count_positive(insns[i].regs_use);

			memcpy(insn->detail->regs_write, insns[i].regs_mod, sizeof(insns[i].regs_mod));
			insn->detail->regs_write_count = (uint8_t)count_positive(insns[i].regs_mod);

			memcpy(insn->detail->groups, insns[i].groups, sizeof(insns[i].groups));
			insn->detail->groups_count = (uint8_t)count_positive8(insns[i].groups);

			if (insns[i].branch || insns[i].indirect_branch) {
				// this insn also belongs to JUMP group. add JUMP group
				insn->detail->groups[insn->detail->groups_count] = XCORE_GRP_JUMP;
				insn->detail->groups_count++;
			}
#endif
		}
	}
}

#ifndef CAPSTONE_DIET
static const name_map insn_name_maps[] = {
	{ XCORE_INS_INVALID, NULL },

	{ XCORE_INS_ADD, "add" },
	{ XCORE_INS_ANDNOT, "andnot" },
	{ XCORE_INS_AND, "and" },
	{ XCORE_INS_ASHR, "ashr" },
	{ XCORE_INS_BAU, "bau" },
	{ XCORE_INS_BITREV, "bitrev" },
	{ XCORE_INS_BLA, "bla" },
	{ XCORE_INS_BLAT, "blat" },
	{ XCORE_INS_BL, "bl" },
	{ XCORE_INS_BF, "bf" },
	{ XCORE_INS_BT, "bt" },
	{ XCORE_INS_BU, "bu" },
	{ XCORE_INS_BRU, "bru" },
	{ XCORE_INS_BYTEREV, "byterev" },
	{ XCORE_INS_CHKCT, "chkct" },
	{ XCORE_INS_CLRE, "clre" },
	{ XCORE_INS_CLRPT, "clrpt" },
	{ XCORE_INS_CLRSR, "clrsr" },
	{ XCORE_INS_CLZ, "clz" },
	{ XCORE_INS_CRC8, "crc8" },
	{ XCORE_INS_CRC32, "crc32" },
	{ XCORE_INS_DCALL, "dcall" },
	{ XCORE_INS_DENTSP, "dentsp" },
	{ XCORE_INS_DGETREG, "dgetreg" },
	{ XCORE_INS_DIVS, "divs" },
	{ XCORE_INS_DIVU, "divu" },
	{ XCORE_INS_DRESTSP, "drestsp" },
	{ XCORE_INS_DRET, "dret" },
	{ XCORE_INS_ECALLF, "ecallf" },
	{ XCORE_INS_ECALLT, "ecallt" },
	{ XCORE_INS_EDU, "edu" },
	{ XCORE_INS_EEF, "eef" },
	{ XCORE_INS_EET, "eet" },
	{ XCORE_INS_EEU, "eeu" },
	{ XCORE_INS_ENDIN, "endin" },
	{ XCORE_INS_ENTSP, "entsp" },
	{ XCORE_INS_EQ, "eq" },
	{ XCORE_INS_EXTDP, "extdp" },
	{ XCORE_INS_EXTSP, "extsp" },
	{ XCORE_INS_FREER, "freer" },
	{ XCORE_INS_FREET, "freet" },
	{ XCORE_INS_GETD, "getd" },
	{ XCORE_INS_GET, "get" },
	{ XCORE_INS_GETN, "getn" },
	{ XCORE_INS_GETR, "getr" },
	{ XCORE_INS_GETSR, "getsr" },
	{ XCORE_INS_GETST, "getst" },
	{ XCORE_INS_GETTS, "getts" },
	{ XCORE_INS_INCT, "inct" },
	{ XCORE_INS_INIT, "init" },
	{ XCORE_INS_INPW, "inpw" },
	{ XCORE_INS_INSHR, "inshr" },
	{ XCORE_INS_INT, "int" },
	{ XCORE_INS_IN, "in" },
	{ XCORE_INS_KCALL, "kcall" },
	{ XCORE_INS_KENTSP, "kentsp" },
	{ XCORE_INS_KRESTSP, "krestsp" },
	{ XCORE_INS_KRET, "kret" },
	{ XCORE_INS_LADD, "ladd" },
	{ XCORE_INS_LD16S, "ld16s" },
	{ XCORE_INS_LD8U, "ld8u" },
	{ XCORE_INS_LDA16, "lda16" },
	{ XCORE_INS_LDAP, "ldap" },
	{ XCORE_INS_LDAW, "ldaw" },
	{ XCORE_INS_LDC, "ldc" },
	{ XCORE_INS_LDW, "ldw" },
	{ XCORE_INS_LDIVU, "ldivu" },
	{ XCORE_INS_LMUL, "lmul" },
	{ XCORE_INS_LSS, "lss" },
	{ XCORE_INS_LSUB, "lsub" },
	{ XCORE_INS_LSU, "lsu" },
	{ XCORE_INS_MACCS, "maccs" },
	{ XCORE_INS_MACCU, "maccu" },
	{ XCORE_INS_MJOIN, "mjoin" },
	{ XCORE_INS_MKMSK, "mkmsk" },
	{ XCORE_INS_MSYNC, "msync" },
	{ XCORE_INS_MUL, "mul" },
	{ XCORE_INS_NEG, "neg" },
	{ XCORE_INS_NOT, "not" },
	{ XCORE_INS_OR, "or" },
	{ XCORE_INS_OUTCT, "outct" },
	{ XCORE_INS_OUTPW, "outpw" },
	{ XCORE_INS_OUTSHR, "outshr" },
	{ XCORE_INS_OUTT, "outt" },
	{ XCORE_INS_OUT, "out" },
	{ XCORE_INS_PEEK, "peek" },
	{ XCORE_INS_REMS, "rems" },
	{ XCORE_INS_REMU, "remu" },
	{ XCORE_INS_RETSP, "retsp" },
	{ XCORE_INS_SETCLK, "setclk" },
	{ XCORE_INS_SET, "set" },
	{ XCORE_INS_SETC, "setc" },
	{ XCORE_INS_SETD, "setd" },
	{ XCORE_INS_SETEV, "setev" },
	{ XCORE_INS_SETN, "setn" },
	{ XCORE_INS_SETPSC, "setpsc" },
	{ XCORE_INS_SETPT, "setpt" },
	{ XCORE_INS_SETRDY, "setrdy" },
	{ XCORE_INS_SETSR, "setsr" },
	{ XCORE_INS_SETTW, "settw" },
	{ XCORE_INS_SETV, "setv" },
	{ XCORE_INS_SEXT, "sext" },
	{ XCORE_INS_SHL, "shl" },
	{ XCORE_INS_SHR, "shr" },
	{ XCORE_INS_SSYNC, "ssync" },
	{ XCORE_INS_ST16, "st16" },
	{ XCORE_INS_ST8, "st8" },
	{ XCORE_INS_STW, "stw" },
	{ XCORE_INS_SUB, "sub" },
	{ XCORE_INS_SYNCR, "syncr" },
	{ XCORE_INS_TESTCT, "testct" },
	{ XCORE_INS_TESTLCL, "testlcl" },
	{ XCORE_INS_TESTWCT, "testwct" },
	{ XCORE_INS_TSETMR, "tsetmr" },
	{ XCORE_INS_START, "start" },
	{ XCORE_INS_WAITEF, "waitef" },
	{ XCORE_INS_WAITET, "waitet" },
	{ XCORE_INS_WAITEU, "waiteu" },
	{ XCORE_INS_XOR, "xor" },
	{ XCORE_INS_ZEXT, "zext" },
};

// special alias insn
static const name_map alias_insn_names[] = {
	{ 0, NULL }
};
#endif

const char *XCore_insn_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	unsigned int i;

	if (id >= XCORE_INS_ENDING)
		return NULL;

	// handle special alias first
	for (i = 0; i < ARR_SIZE(alias_insn_names); i++) {
		if (alias_insn_names[i].id == id)
			return alias_insn_names[i].name;
	}

	return insn_name_maps[id].name;
#else
	return NULL;
#endif
}

#ifndef CAPSTONE_DIET
static const name_map group_name_maps[] = {
	{ XCORE_GRP_INVALID, NULL },
	{ XCORE_GRP_JUMP, "jump" },
};
#endif

const char *XCore_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(group_name_maps, ARR_SIZE(group_name_maps), id);
#else
	return NULL;
#endif
}

// map internal raw register to 'public' register
xcore_reg XCore_map_register(unsigned int r)
{
	static const unsigned int map[] = { 0,
	};

	if (r < ARR_SIZE(map))
		return map[r];

	// cannot find this register
	return 0;
}

#endif
