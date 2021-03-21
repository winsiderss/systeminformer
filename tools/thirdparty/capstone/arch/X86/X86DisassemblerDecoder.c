/*===-- X86DisassemblerDecoder.c - Disassembler decoder ------------*- C -*-===*
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 *===----------------------------------------------------------------------===*
 *
 * This file is part of the X86 Disassembler.
 * It contains the implementation of the instruction decoder.
 * Documentation for the disassembler can be found in X86Disassembler.h.
 *
 *===----------------------------------------------------------------------===*/

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_X86

#include <stdarg.h>   /* for va_*()       */
#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <libkern/libkern.h>
#else
#include <stdlib.h>   /* for exit()       */
#endif

#include "../../cs_priv.h"
#include "../../utils.h"

#include "X86DisassemblerDecoder.h"

/// Specifies whether a ModR/M byte is needed and (if so) which
/// instruction each possible value of the ModR/M byte corresponds to.  Once
/// this information is known, we have narrowed down to a single instruction.
struct ModRMDecision {
	uint8_t modrm_type;
	uint16_t instructionIDs;
};

/// Specifies which set of ModR/M->instruction tables to look at
/// given a particular opcode.
struct OpcodeDecision {
	struct ModRMDecision modRMDecisions[256];
};

/// Specifies which opcode->instruction tables to look at given
/// a particular context (set of attributes).  Since there are many possible
/// contexts, the decoder first uses CONTEXTS_SYM to determine which context
/// applies given a specific set of attributes.  Hence there are only IC_max
/// entries in this table, rather than 2^(ATTR_max).
struct ContextDecision {
	struct OpcodeDecision opcodeDecisions[IC_max];
};

#ifdef CAPSTONE_X86_REDUCE
#include "X86GenDisassemblerTables_reduce.inc"
#else
#include "X86GenDisassemblerTables.inc"
#endif

//#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_DESC
#ifdef CAPSTONE_X86_REDUCE
#include "X86GenInstrInfo_reduce.inc"
#else
#include "X86GenInstrInfo.inc"
#endif

/*
 * contextForAttrs - Client for the instruction context table.  Takes a set of
 *   attributes and returns the appropriate decode context.
 *
 * @param attrMask  - Attributes, from the enumeration attributeBits.
 * @return          - The InstructionContext to use when looking up an
 *                    an instruction with these attributes.
 */
static InstructionContext contextForAttrs(uint16_t attrMask)
{
	return CONTEXTS_SYM[attrMask];
}

/*
 * modRMRequired - Reads the appropriate instruction table to determine whether
 *   the ModR/M byte is required to decode a particular instruction.
 *
 * @param type        - The opcode type (i.e., how many bytes it has).
 * @param insnContext - The context for the instruction, as returned by
 *                      contextForAttrs.
 * @param opcode      - The last byte of the instruction's opcode, not counting
 *                      ModR/M extensions and escapes.
 * @return            - true if the ModR/M byte is required, false otherwise.
 */
static int modRMRequired(OpcodeType type,
		InstructionContext insnContext,
		uint16_t opcode)
{
	const struct OpcodeDecision *decision = NULL;
	const uint8_t *indextable = NULL;
	uint8_t index;

	switch (type) {
		default:
		case ONEBYTE:
			decision = ONEBYTE_SYM;
			indextable = index_x86DisassemblerOneByteOpcodes;
			break;
		case TWOBYTE:
			decision = TWOBYTE_SYM;
			indextable = index_x86DisassemblerTwoByteOpcodes;
			break;
		case THREEBYTE_38:
			decision = THREEBYTE38_SYM;
			indextable = index_x86DisassemblerThreeByte38Opcodes;
			break;
		case THREEBYTE_3A:
			decision = THREEBYTE3A_SYM;
			indextable = index_x86DisassemblerThreeByte3AOpcodes;
			break;
#ifndef CAPSTONE_X86_REDUCE
		case XOP8_MAP:
			decision = XOP8_MAP_SYM;
			indextable = index_x86DisassemblerXOP8Opcodes;
			break;
		case XOP9_MAP:
			decision = XOP9_MAP_SYM;
			indextable = index_x86DisassemblerXOP9Opcodes;
			break;
		case XOPA_MAP:
			decision = XOPA_MAP_SYM;
			indextable = index_x86DisassemblerXOPAOpcodes;
			break;
		case T3DNOW_MAP:
			// 3DNow instructions always have ModRM byte
			return true;
#endif
	}

	index = indextable[insnContext];
	if (index)
		return decision[index - 1].modRMDecisions[opcode].modrm_type != MODRM_ONEENTRY;
	else
		return false;
}

/*
 * decode - Reads the appropriate instruction table to obtain the unique ID of
 *   an instruction.
 *
 * @param type        - See modRMRequired().
 * @param insnContext - See modRMRequired().
 * @param opcode      - See modRMRequired().
 * @param modRM       - The ModR/M byte if required, or any value if not.
 * @return            - The UID of the instruction, or 0 on failure.
 */
static InstrUID decode(OpcodeType type,
		InstructionContext insnContext,
		uint8_t opcode,
		uint8_t modRM)
{
	const struct ModRMDecision *dec = NULL;
	const uint8_t *indextable = NULL;
	uint8_t index;

	switch (type) {
		default:
		case ONEBYTE:
			indextable = index_x86DisassemblerOneByteOpcodes;
			index = indextable[insnContext];
			if (index)
				dec = &ONEBYTE_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case TWOBYTE:
			indextable = index_x86DisassemblerTwoByteOpcodes;
			index = indextable[insnContext];
			if (index)
				dec = &TWOBYTE_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case THREEBYTE_38:
			indextable = index_x86DisassemblerThreeByte38Opcodes;
			index = indextable[insnContext];
			if (index)
				dec = &THREEBYTE38_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case THREEBYTE_3A:
			indextable = index_x86DisassemblerThreeByte3AOpcodes;
			index = indextable[insnContext];
			if (index)
				dec = &THREEBYTE3A_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
#ifndef CAPSTONE_X86_REDUCE
		case XOP8_MAP:
			indextable = index_x86DisassemblerXOP8Opcodes;
			index = indextable[insnContext];
			if (index)
				dec = &XOP8_MAP_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case XOP9_MAP:
			indextable = index_x86DisassemblerXOP9Opcodes;
			index = indextable[insnContext];
			if (index)
				dec = &XOP9_MAP_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case XOPA_MAP:
			indextable = index_x86DisassemblerXOPAOpcodes;
			index = indextable[insnContext];
			if (index)
				dec = &XOPA_MAP_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
		case T3DNOW_MAP:
			indextable = index_x86DisassemblerT3DNOWOpcodes;
			index = indextable[insnContext];
			if (index)
				dec = &T3DNOW_MAP_SYM[index - 1].modRMDecisions[opcode];
			else
				dec = &emptyTable.modRMDecisions[opcode];
			break;
#endif
	}

	switch (dec->modrm_type) {
		default:
			//debug("Corrupt table!  Unknown modrm_type");
			return 0;
		case MODRM_ONEENTRY:
			return modRMTable[dec->instructionIDs];
		case MODRM_SPLITRM:
			if (modFromModRM(modRM) == 0x3)
				return modRMTable[dec->instructionIDs+1];
			return modRMTable[dec->instructionIDs];
		case MODRM_SPLITREG:
			if (modFromModRM(modRM) == 0x3)
				return modRMTable[dec->instructionIDs+((modRM & 0x38) >> 3)+8];
			return modRMTable[dec->instructionIDs+((modRM & 0x38) >> 3)];
		case MODRM_SPLITMISC:
			if (modFromModRM(modRM) == 0x3)
				return modRMTable[dec->instructionIDs+(modRM & 0x3f)+8];
			return modRMTable[dec->instructionIDs+((modRM & 0x38) >> 3)];
		case MODRM_FULL:
			return modRMTable[dec->instructionIDs+modRM];
	}
}

/*
 * specifierForUID - Given a UID, returns the name and operand specification for
 *   that instruction.
 *
 * @param uid - The unique ID for the instruction.  This should be returned by
 *              decode(); specifierForUID will not check bounds.
 * @return    - A pointer to the specification for that instruction.
 */
static const struct InstructionSpecifier *specifierForUID(InstrUID uid)
{
	return &INSTRUCTIONS_SYM[uid];
}

/*
 * consumeByte - Uses the reader function provided by the user to consume one
 *   byte from the instruction's memory and advance the cursor.
 *
 * @param insn  - The instruction with the reader function to use.  The cursor
 *                for this instruction is advanced.
 * @param byte  - A pointer to a pre-allocated memory buffer to be populated
 *                with the data read.
 * @return      - 0 if the read was successful; nonzero otherwise.
 */
static int consumeByte(struct InternalInstruction *insn, uint8_t *byte)
{
	int ret = insn->reader(insn->readerArg, byte, insn->readerCursor);

	if (!ret)
		++(insn->readerCursor);

	return ret;
}

/*
 * lookAtByte - Like consumeByte, but does not advance the cursor.
 *
 * @param insn  - See consumeByte().
 * @param byte  - See consumeByte().
 * @return      - See consumeByte().
 */
static int lookAtByte(struct InternalInstruction *insn, uint8_t *byte)
{
	return insn->reader(insn->readerArg, byte, insn->readerCursor);
}

static void unconsumeByte(struct InternalInstruction *insn)
{
	insn->readerCursor--;
}

#define CONSUME_FUNC(name, type)                                  \
	static int name(struct InternalInstruction *insn, type *ptr) {  \
		type combined = 0;                                            \
		unsigned offset;                                              \
		for (offset = 0; offset < sizeof(type); ++offset) {           \
			uint8_t byte;                                               \
			int ret = insn->reader(insn->readerArg,                     \
					&byte,                               \
					insn->readerCursor + offset);        \
			if (ret)                                                    \
			return ret;                                               \
			combined = combined | (type)((uint64_t)byte << (offset * 8));     \
		}                                                             \
		*ptr = combined;                                              \
		insn->readerCursor += sizeof(type);                           \
		return 0;                                                     \
	}

/*
 * consume* - Use the reader function provided by the user to consume data
 *   values of various sizes from the instruction's memory and advance the
 *   cursor appropriately.  These readers perform endian conversion.
 *
 * @param insn    - See consumeByte().
 * @param ptr     - A pointer to a pre-allocated memory of appropriate size to
 *                  be populated with the data read.
 * @return        - See consumeByte().
 */
CONSUME_FUNC(consumeInt8, int8_t)
CONSUME_FUNC(consumeInt16, int16_t)
CONSUME_FUNC(consumeInt32, int32_t)
CONSUME_FUNC(consumeUInt16, uint16_t)
CONSUME_FUNC(consumeUInt32, uint32_t)
CONSUME_FUNC(consumeUInt64, uint64_t)

/*
 * setPrefixPresent - Marks that a particular prefix is present at a particular
 *   location.
 *
 * @param insn      - The instruction to be marked as having the prefix.
 * @param prefix    - The prefix that is present.
 * @param location  - The location where the prefix is located (in the address
 *                    space of the instruction's reader).
 */
static void setPrefixPresent(struct InternalInstruction *insn, uint8_t prefix, uint64_t location)
{
	switch (prefix) {
	case 0x26:
		insn->isPrefix26 = true;
		insn->prefix26 = location;
		break;
	case 0x2e:
		insn->isPrefix2e = true;
		insn->prefix2e = location;
		break;
	case 0x36:
		insn->isPrefix36 = true;
		insn->prefix36 = location;
		break;
	case 0x3e:
		insn->isPrefix3e = true;
		insn->prefix3e = location;
		break;
	case 0x64:
		insn->isPrefix64 = true;
		insn->prefix64 = location;
		break;
	case 0x65:
		insn->isPrefix65 = true;
		insn->prefix65 = location;
		break;
	case 0x66:
		insn->isPrefix66 = true;
		insn->prefix66 = location;
		break;
	case 0x67:
		insn->isPrefix67 = true;
		insn->prefix67 = location;
		break;
	case 0xf0:
		insn->isPrefixf0 = true;
		insn->prefixf0 = location;
		break;
	case 0xf2:
		insn->isPrefixf2 = true;
		insn->prefixf2 = location;
		break;
	case 0xf3:
		insn->isPrefixf3 = true;
		insn->prefixf3 = location;
		break;
	default:
		break;
	}
}

/*
 * isPrefixAtLocation - Queries an instruction to determine whether a prefix is
 *   present at a given location.
 *
 * @param insn      - The instruction to be queried.
 * @param prefix    - The prefix.
 * @param location  - The location to query.
 * @return          - Whether the prefix is at that location.
 */
static bool isPrefixAtLocation(struct InternalInstruction *insn, uint8_t prefix,
		uint64_t location)
{
	switch (prefix) {
	case 0x26:
		if (insn->isPrefix26 && insn->prefix26 == location)
			return true;
		break;
	case 0x2e:
		if (insn->isPrefix2e && insn->prefix2e == location)
			return true;
		break;
	case 0x36:
		if (insn->isPrefix36 && insn->prefix36 == location)
			return true;
		break;
	case 0x3e:
		if (insn->isPrefix3e && insn->prefix3e == location)
			return true;
		break;
	case 0x64:
		if (insn->isPrefix64 && insn->prefix64 == location)
			return true;
		break;
	case 0x65:
		if (insn->isPrefix65 && insn->prefix65 == location)
			return true;
		break;
	case 0x66:
		if (insn->isPrefix66 && insn->prefix66 == location)
			return true;
		break;
	case 0x67:
		if (insn->isPrefix67 && insn->prefix67 == location)
			return true;
		break;
	case 0xf0:
		if (insn->isPrefixf0 && insn->prefixf0 == location)
			return true;
		break;
	case 0xf2:
		if (insn->isPrefixf2 && insn->prefixf2 == location)
			return true;
		break;
	case 0xf3:
		if (insn->isPrefixf3 && insn->prefixf3 == location)
			return true;
		break;
	default:
		break;
	}
	return false;
}

/*
 * readPrefixes - Consumes all of an instruction's prefix bytes, and marks the
 *   instruction as having them.  Also sets the instruction's default operand,
 *   address, and other relevant data sizes to report operands correctly.
 *
 * @param insn  - The instruction whose prefixes are to be read.
 * @return      - 0 if the instruction could be read until the end of the prefix
 *                bytes, and no prefixes conflicted; nonzero otherwise.
 */
static int readPrefixes(struct InternalInstruction *insn)
{
	bool isPrefix = true;
	uint64_t prefixLocation;
	uint8_t byte = 0, nextByte;

	bool hasAdSize = false;
	bool hasOpSize = false;

	//initialize to an impossible value
	insn->necessaryPrefixLocation = insn->readerCursor - 1;
	while (isPrefix) {
		if (insn->mode == MODE_64BIT) {
			// eliminate consecutive redundant REX bytes in front
			if (consumeByte(insn, &byte))
				return -1;

			if ((byte & 0xf0) == 0x40) {
				while(true) {
					if (lookAtByte(insn, &byte))	// out of input code
						return -1;
					if ((byte & 0xf0) == 0x40) {
						// another REX prefix, but we only remember the last one
						if (consumeByte(insn, &byte))
							return -1;
					} else
						break;
				}

				// recover the last REX byte if next byte is not a legacy prefix
				switch (byte) {
					case 0xf2:  /* REPNE/REPNZ */
					case 0xf3:  /* REP or REPE/REPZ */
					case 0xf0:  /* LOCK */
					case 0x2e:  /* CS segment override -OR- Branch not taken */
					case 0x36:  /* SS segment override -OR- Branch taken */
					case 0x3e:  /* DS segment override */
					case 0x26:  /* ES segment override */
					case 0x64:  /* FS segment override */
					case 0x65:  /* GS segment override */
					case 0x66:  /* Operand-size override */
					case 0x67:  /* Address-size override */
						break;
					default:    /* Not a prefix byte */
						unconsumeByte(insn);
						break;
				}
			} else {
				unconsumeByte(insn);
			}
		}

		prefixLocation = insn->readerCursor;

		/* If we fail reading prefixes, just stop here and let the opcode reader deal with it */
		if (consumeByte(insn, &byte))
			return -1;

		if (insn->readerCursor - 1 == insn->startLocation
				&& (byte == 0xf2 || byte == 0xf3)) {

			if (lookAtByte(insn, &nextByte))
				return -1;

			/*
			 * If the byte is 0xf2 or 0xf3, and any of the following conditions are
			 * met:
			 * - it is followed by a LOCK (0xf0) prefix
			 * - it is followed by an xchg instruction
			 * then it should be disassembled as a xacquire/xrelease not repne/rep.
			 */
			if (((nextByte == 0xf0) ||
				((nextByte & 0xfe) == 0x86 || (nextByte & 0xf8) == 0x90)))
				insn->xAcquireRelease = byte;

			/*
			 * Also if the byte is 0xf3, and the following condition is met:
			 * - it is followed by a "mov mem, reg" (opcode 0x88/0x89) or
			 *                       "mov mem, imm" (opcode 0xc6/0xc7) instructions.
			 * then it should be disassembled as an xrelease not rep.
			 */
			if (byte == 0xf3 &&
					(nextByte == 0x88 || nextByte == 0x89 ||
					 nextByte == 0xc6 || nextByte == 0xc7))
				insn->xAcquireRelease = byte;

			if (insn->mode == MODE_64BIT && (nextByte & 0xf0) == 0x40) {
				if (consumeByte(insn, &nextByte))
					return -1;
				if (lookAtByte(insn, &nextByte))
					return -1;
				unconsumeByte(insn);
			}
		}

		switch (byte) {
			case 0xf2:  /* REPNE/REPNZ */
			case 0xf3:  /* REP or REPE/REPZ */
			case 0xf0:  /* LOCK */
				// only accept the last prefix
				insn->isPrefixf2 = false;
				insn->isPrefixf3 = false;
				insn->isPrefixf0 = false;
				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix0 = byte;
				break;
			case 0x2e:  /* CS segment override -OR- Branch not taken */
				insn->segmentOverride = SEG_OVERRIDE_CS;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x36:  /* SS segment override -OR- Branch taken */
				insn->segmentOverride = SEG_OVERRIDE_SS;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x3e:  /* DS segment override */
				insn->segmentOverride = SEG_OVERRIDE_DS;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x26:  /* ES segment override */
				insn->segmentOverride = SEG_OVERRIDE_ES;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x64:  /* FS segment override */
				insn->segmentOverride = SEG_OVERRIDE_FS;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x65:  /* GS segment override */
				insn->segmentOverride = SEG_OVERRIDE_GS;
				// only accept the last prefix
				insn->isPrefix2e = false;
				insn->isPrefix36 = false;
				insn->isPrefix3e = false;
				insn->isPrefix26 = false;
				insn->isPrefix64 = false;
				insn->isPrefix65 = false;

				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix1 = byte;
				break;
			case 0x66:  /* Operand-size override */
				hasOpSize = true;
				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix2 = byte;
				break;
			case 0x67:  /* Address-size override */
				hasAdSize = true;
				setPrefixPresent(insn, byte, prefixLocation);
				insn->prefix3 = byte;
				break;
			default:    /* Not a prefix byte */
				isPrefix = false;
				break;
		}

		//if (isPrefix)
		//	dbgprintf(insn, "Found prefix 0x%hhx", byte);
	}

	insn->vectorExtensionType = TYPE_NO_VEX_XOP;


	if (byte == 0x62) {
		uint8_t byte1, byte2;

		if (consumeByte(insn, &byte1)) {
			//dbgprintf(insn, "Couldn't read second byte of EVEX prefix");
			return -1;
		}

		if ((insn->mode == MODE_64BIT || (byte1 & 0xc0) == 0xc0) &&
				((~byte1 & 0xc) == 0xc)) {
			if (lookAtByte(insn, &byte2)) {
				//dbgprintf(insn, "Couldn't read third byte of EVEX prefix");
				return -1;
			}

			if ((byte2 & 0x4) == 0x4) {
				insn->vectorExtensionType = TYPE_EVEX;
			} else {
				unconsumeByte(insn); /* unconsume byte1 */
				unconsumeByte(insn); /* unconsume byte  */
				insn->necessaryPrefixLocation = insn->readerCursor - 2;
			}

			if (insn->vectorExtensionType == TYPE_EVEX) {
				insn->vectorExtensionPrefix[0] = byte;
				insn->vectorExtensionPrefix[1] = byte1;

				if (consumeByte(insn, &insn->vectorExtensionPrefix[2])) {
					//dbgprintf(insn, "Couldn't read third byte of EVEX prefix");
					return -1;
				}

				if (consumeByte(insn, &insn->vectorExtensionPrefix[3])) {
					//dbgprintf(insn, "Couldn't read fourth byte of EVEX prefix");
					return -1;
				}

				/* We simulate the REX prefix for simplicity's sake */
				if (insn->mode == MODE_64BIT) {
					insn->rexPrefix = 0x40
						| (wFromEVEX3of4(insn->vectorExtensionPrefix[2]) << 3)
						| (rFromEVEX2of4(insn->vectorExtensionPrefix[1]) << 2)
						| (xFromEVEX2of4(insn->vectorExtensionPrefix[1]) << 1)
						| (bFromEVEX2of4(insn->vectorExtensionPrefix[1]) << 0);
				}
				switch (ppFromEVEX3of4(insn->vectorExtensionPrefix[2])) {
					default:
						break;
					case VEX_PREFIX_66:
						hasOpSize = true;
						break;
				}
				//dbgprintf(insn, "Found EVEX prefix 0x%hhx 0x%hhx 0x%hhx 0x%hhx",
				//		insn->vectorExtensionPrefix[0], insn->vectorExtensionPrefix[1],
				//		insn->vectorExtensionPrefix[2], insn->vectorExtensionPrefix[3]);
			}
		} else {
			// BOUND instruction
			unconsumeByte(insn); /* unconsume byte1 */
			unconsumeByte(insn); /* unconsume byte */
		}
	} else if (byte == 0xc4) {
		uint8_t byte1;

		if (lookAtByte(insn, &byte1)) {
			//dbgprintf(insn, "Couldn't read second byte of VEX");
			return -1;
		}

		if (insn->mode == MODE_64BIT || (byte1 & 0xc0) == 0xc0) {
			insn->vectorExtensionType = TYPE_VEX_3B;
			insn->necessaryPrefixLocation = insn->readerCursor - 1;
		} else {
			unconsumeByte(insn);
			insn->necessaryPrefixLocation = insn->readerCursor - 1;
		}

		if (insn->vectorExtensionType == TYPE_VEX_3B) {
			insn->vectorExtensionPrefix[0] = byte;
			if (consumeByte(insn, &insn->vectorExtensionPrefix[1]))
				return -1;
			if (consumeByte(insn, &insn->vectorExtensionPrefix[2]))
				return -1;

			/* We simulate the REX prefix for simplicity's sake */
			if (insn->mode == MODE_64BIT) {
				insn->rexPrefix = 0x40
					| (wFromVEX3of3(insn->vectorExtensionPrefix[2]) << 3)
					| (rFromVEX2of3(insn->vectorExtensionPrefix[1]) << 2)
					| (xFromVEX2of3(insn->vectorExtensionPrefix[1]) << 1)
					| (bFromVEX2of3(insn->vectorExtensionPrefix[1]) << 0);

			}
			switch (ppFromVEX3of3(insn->vectorExtensionPrefix[2])) {
				default:
					break;
				case VEX_PREFIX_66:
					hasOpSize = true;
					break;
			}
		}
	} else if (byte == 0xc5) {
		uint8_t byte1;

		if (lookAtByte(insn, &byte1)) {
			//dbgprintf(insn, "Couldn't read second byte of VEX");
			return -1;
		}

		if (insn->mode == MODE_64BIT || (byte1 & 0xc0) == 0xc0) {
			insn->vectorExtensionType = TYPE_VEX_2B;
		} else {
			unconsumeByte(insn);
		}

		if (insn->vectorExtensionType == TYPE_VEX_2B) {
			insn->vectorExtensionPrefix[0] = byte;
			if (consumeByte(insn, &insn->vectorExtensionPrefix[1]))
				return -1;

			if (insn->mode == MODE_64BIT) {
				insn->rexPrefix = 0x40
					| (rFromVEX2of2(insn->vectorExtensionPrefix[1]) << 2);
			}

			switch (ppFromVEX2of2(insn->vectorExtensionPrefix[1])) {
				default:
					break;
				case VEX_PREFIX_66:
					hasOpSize = true;
					break;
			}
		}
	} else if (byte == 0x8f) {
		uint8_t byte1;

		if (lookAtByte(insn, &byte1)) {
			// dbgprintf(insn, "Couldn't read second byte of XOP");
			return -1;
		}

		if ((byte1 & 0x38) != 0x0) { /* 0 in these 3 bits is a POP instruction. */
			insn->vectorExtensionType = TYPE_XOP;
			insn->necessaryPrefixLocation = insn->readerCursor - 1;
		} else {
			unconsumeByte(insn);
			insn->necessaryPrefixLocation = insn->readerCursor - 1;
		}

		if (insn->vectorExtensionType == TYPE_XOP) {
			insn->vectorExtensionPrefix[0] = byte;
			if (consumeByte(insn, &insn->vectorExtensionPrefix[1]))
				return -1;
			if (consumeByte(insn, &insn->vectorExtensionPrefix[2]))
				return -1;

			/* We simulate the REX prefix for simplicity's sake */
			if (insn->mode == MODE_64BIT) {
				insn->rexPrefix = 0x40
					| (wFromXOP3of3(insn->vectorExtensionPrefix[2]) << 3)
					| (rFromXOP2of3(insn->vectorExtensionPrefix[1]) << 2)
					| (xFromXOP2of3(insn->vectorExtensionPrefix[1]) << 1)
					| (bFromXOP2of3(insn->vectorExtensionPrefix[1]) << 0);
			}

			switch (ppFromXOP3of3(insn->vectorExtensionPrefix[2])) {
				default:
					break;
				case VEX_PREFIX_66:
					hasOpSize = true;
					break;
			}
		}
	} else {
		if (insn->mode == MODE_64BIT) {
			if ((byte & 0xf0) == 0x40) {
				uint8_t opcodeByte;

				while(true) {
					if (lookAtByte(insn, &opcodeByte))	// out of input code
						return -1;
					if ((opcodeByte & 0xf0) == 0x40) {
						// another REX prefix, but we only remember the last one
						if (consumeByte(insn, &byte))
							return -1;
					} else
						break;
				}

				insn->rexPrefix = byte;
				insn->necessaryPrefixLocation = insn->readerCursor - 2;
				// dbgprintf(insn, "Found REX prefix 0x%hhx", byte);
			} else {
				unconsumeByte(insn);
				insn->necessaryPrefixLocation = insn->readerCursor - 1;
			}
		} else {
			unconsumeByte(insn);
			insn->necessaryPrefixLocation = insn->readerCursor - 1;
		}
	}

	if (insn->mode == MODE_16BIT) {
		insn->registerSize       = (hasOpSize ? 4 : 2);
		insn->addressSize        = (hasAdSize ? 4 : 2);
		insn->displacementSize   = (hasAdSize ? 4 : 2);
		insn->immediateSize      = (hasOpSize ? 4 : 2);
		insn->immSize = (hasOpSize ? 4 : 2);
	} else if (insn->mode == MODE_32BIT) {
		insn->registerSize       = (hasOpSize ? 2 : 4);
		insn->addressSize        = (hasAdSize ? 2 : 4);
		insn->displacementSize   = (hasAdSize ? 2 : 4);
		insn->immediateSize      = (hasOpSize ? 2 : 4);
		insn->immSize = (hasOpSize ? 2 : 4);
	} else if (insn->mode == MODE_64BIT) {
		if (insn->rexPrefix && wFromREX(insn->rexPrefix)) {
			insn->registerSize       = 8;
			insn->addressSize        = (hasAdSize ? 4 : 8);
			insn->displacementSize   = 4;
			insn->immediateSize      = 4;
			insn->immSize      = 4;
		} else if (insn->rexPrefix) {
			insn->registerSize       = (hasOpSize ? 2 : 4);
			insn->addressSize        = (hasAdSize ? 4 : 8);
			insn->displacementSize   = (hasOpSize ? 2 : 4);
			insn->immediateSize      = (hasOpSize ? 2 : 4);
			insn->immSize      = (hasOpSize ? 2 : 4);
		} else {
			insn->registerSize       = (hasOpSize ? 2 : 4);
			insn->addressSize        = (hasAdSize ? 4 : 8);
			insn->displacementSize   = (hasOpSize ? 2 : 4);
			insn->immediateSize      = (hasOpSize ? 2 : 4);
			insn->immSize      = (hasOpSize ? 4 : 8);
		}
	}

	return 0;
}

static int readModRM(struct InternalInstruction *insn);

/*
 * readOpcode - Reads the opcode (excepting the ModR/M byte in the case of
 *   extended or escape opcodes).
 *
 * @param insn  - The instruction whose opcode is to be read.
 * @return      - 0 if the opcode could be read successfully; nonzero otherwise.
 */
static int readOpcode(struct InternalInstruction *insn)
{
	/* Determine the length of the primary opcode */
	uint8_t current;

	// printf(">>> readOpcode() = %x\n", insn->readerCursor);

	insn->opcodeType = ONEBYTE;
	insn->firstByte = 0x00;

	if (insn->vectorExtensionType == TYPE_EVEX) {
		switch (mmFromEVEX2of4(insn->vectorExtensionPrefix[1])) {
			default:
				// dbgprintf(insn, "Unhandled mm field for instruction (0x%hhx)",
				// 		mmFromEVEX2of4(insn->vectorExtensionPrefix[1]));
				return -1;
			case VEX_LOB_0F:
				insn->opcodeType = TWOBYTE;
				return consumeByte(insn, &insn->opcode);
			case VEX_LOB_0F38:
				insn->opcodeType = THREEBYTE_38;
				return consumeByte(insn, &insn->opcode);
			case VEX_LOB_0F3A:
				insn->opcodeType = THREEBYTE_3A;
				return consumeByte(insn, &insn->opcode);
		}
	} else if (insn->vectorExtensionType == TYPE_VEX_3B) {
		switch (mmmmmFromVEX2of3(insn->vectorExtensionPrefix[1])) {
			default:
				// dbgprintf(insn, "Unhandled m-mmmm field for instruction (0x%hhx)",
				//		mmmmmFromVEX2of3(insn->vectorExtensionPrefix[1]));
				return -1;
			case VEX_LOB_0F:
				insn->twoByteEscape = 0x0f;
				insn->opcodeType = TWOBYTE;
				return consumeByte(insn, &insn->opcode);
			case VEX_LOB_0F38:
				insn->twoByteEscape = 0x0f;
				insn->threeByteEscape = 0x38;
				insn->opcodeType = THREEBYTE_38;
				return consumeByte(insn, &insn->opcode);
			case VEX_LOB_0F3A:
				insn->twoByteEscape = 0x0f;
				insn->threeByteEscape = 0x3a;
				insn->opcodeType = THREEBYTE_3A;
				return consumeByte(insn, &insn->opcode);
		}
	} else if (insn->vectorExtensionType == TYPE_VEX_2B) {
		insn->twoByteEscape = 0x0f;
		insn->opcodeType = TWOBYTE;
		return consumeByte(insn, &insn->opcode);
	} else if (insn->vectorExtensionType == TYPE_XOP) {
		switch (mmmmmFromXOP2of3(insn->vectorExtensionPrefix[1])) {
			default:
				// dbgprintf(insn, "Unhandled m-mmmm field for instruction (0x%hhx)",
				// 		mmmmmFromVEX2of3(insn->vectorExtensionPrefix[1]));
				return -1;
			case XOP_MAP_SELECT_8:
				// FIXME: twoByteEscape?
				insn->opcodeType = XOP8_MAP;
				return consumeByte(insn, &insn->opcode);
			case XOP_MAP_SELECT_9:
				// FIXME: twoByteEscape?
				insn->opcodeType = XOP9_MAP;
				return consumeByte(insn, &insn->opcode);
			case XOP_MAP_SELECT_A:
				// FIXME: twoByteEscape?
				insn->opcodeType = XOPA_MAP;
				return consumeByte(insn, &insn->opcode);
		}
	}

	if (consumeByte(insn, &current))
		return -1;

	// save this first byte for MOVcr, MOVdr, MOVrc, MOVrd
	insn->firstByte = current;

	if (current == 0x0f) {
		// dbgprintf(insn, "Found a two-byte escape prefix (0x%hhx)", current);

		insn->twoByteEscape = current;

		if (consumeByte(insn, &current))
			return -1;

		if (current == 0x38) {
			// dbgprintf(insn, "Found a three-byte escape prefix (0x%hhx)", current);

			insn->threeByteEscape = current;

			if (consumeByte(insn, &current))
				return -1;

			insn->opcodeType = THREEBYTE_38;
		} else if (current == 0x3a) {
			// dbgprintf(insn, "Found a three-byte escape prefix (0x%hhx)", current);

			insn->threeByteEscape = current;

			if (consumeByte(insn, &current))
				return -1;

			insn->opcodeType = THREEBYTE_3A;
		} else {
#ifndef CAPSTONE_X86_REDUCE
			switch(current) {
				default:
					// dbgprintf(insn, "Didn't find a three-byte escape prefix");
					insn->opcodeType = TWOBYTE;
					break;
				case 0x0e:	// HACK for femms. to be handled properly in next version 3.x
					insn->opcodeType = T3DNOW_MAP;
					// this encode does not have ModRM
					insn->consumedModRM = true;
					break;
				case 0x0f:
					// 3DNow instruction has weird format: ModRM/SIB/displacement + opcode
					if (readModRM(insn))
						return -1;
					// next is 3DNow opcode
					if (consumeByte(insn, &current))
						return -1;
					insn->opcodeType = T3DNOW_MAP;
					break;
			}
#endif
		}
	}

	/*
	 * At this point we have consumed the full opcode.
	 * Anything we consume from here on must be unconsumed.
	 */

	insn->opcode = current;

	return 0;
}

// Hacky for FEMMS
#define GET_INSTRINFO_ENUM
#ifndef CAPSTONE_X86_REDUCE
#include "X86GenInstrInfo.inc"
#else
#include "X86GenInstrInfo_reduce.inc"
#endif

/*
 * getIDWithAttrMask - Determines the ID of an instruction, consuming
 *   the ModR/M byte as appropriate for extended and escape opcodes,
 *   and using a supplied attribute mask.
 *
 * @param instructionID - A pointer whose target is filled in with the ID of the
 *                        instruction.
 * @param insn          - The instruction whose ID is to be determined.
 * @param attrMask      - The attribute mask to search.
 * @return              - 0 if the ModR/M could be read when needed or was not
 *                        needed; nonzero otherwise.
 */
static int getIDWithAttrMask(uint16_t *instructionID,
		struct InternalInstruction *insn,
		uint16_t attrMask)
{
	bool hasModRMExtension;

	InstructionContext instructionClass;

#ifndef CAPSTONE_X86_REDUCE
	// HACK for femms. to be handled properly in next version 3.x
	if (insn->opcode == 0x0e && insn->opcodeType == T3DNOW_MAP) {
		*instructionID = X86_FEMMS;
		return 0;
	}
#endif

	if (insn->opcodeType == T3DNOW_MAP)
		instructionClass = IC_OF;
	else
		instructionClass = contextForAttrs(attrMask);

	hasModRMExtension = modRMRequired(insn->opcodeType,
			instructionClass,
			insn->opcode) != 0;

	if (hasModRMExtension) {
		if (readModRM(insn))
			return -1;

		*instructionID = decode(insn->opcodeType,
				instructionClass,
				insn->opcode,
				insn->modRM);
	} else {
		*instructionID = decode(insn->opcodeType,
				instructionClass,
				insn->opcode,
				0);
	}

	return 0;
}

/*
 * is16BitEquivalent - Determines whether two instruction names refer to
 * equivalent instructions but one is 16-bit whereas the other is not.
 *
 * @param orig  - The instruction ID that is not 16-bit
 * @param equiv - The instruction ID that is 16-bit
 */
static bool is16BitEquivalent(unsigned orig, unsigned equiv)
{
	size_t i;
	uint16_t idx;

	if ((idx = x86_16_bit_eq_lookup[orig]) != 0) {
		for (i = idx - 1; i < ARR_SIZE(x86_16_bit_eq_tbl) && x86_16_bit_eq_tbl[i].first == orig; i++) {
			if (x86_16_bit_eq_tbl[i].second == equiv)
				return true;
		}
	}

	return false;
}

/*
 * is64Bit - Determines whether this instruction is a 64-bit instruction.
 *
 * @param name - The instruction that is not 16-bit
 */
static bool is64Bit(uint16_t id)
{
	return is_64bit_insn[id];
}

/*
 * getID - Determines the ID of an instruction, consuming the ModR/M byte as
 *   appropriate for extended and escape opcodes.  Determines the attributes and
 *   context for the instruction before doing so.
 *
 * @param insn  - The instruction whose ID is to be determined.
 * @return      - 0 if the ModR/M could be read when needed or was not needed;
 *                nonzero otherwise.
 */
static int getID(struct InternalInstruction *insn)
{
	uint16_t attrMask;
	uint16_t instructionID;

	// printf(">>> getID()\n");
	attrMask = ATTR_NONE;

	if (insn->mode == MODE_64BIT)
		attrMask |= ATTR_64BIT;

	if (insn->vectorExtensionType != TYPE_NO_VEX_XOP) {
		attrMask |= (insn->vectorExtensionType == TYPE_EVEX) ? ATTR_EVEX : ATTR_VEX;

		if (insn->vectorExtensionType == TYPE_EVEX) {
			switch (ppFromEVEX3of4(insn->vectorExtensionPrefix[2])) {
				case VEX_PREFIX_66:
					attrMask |= ATTR_OPSIZE;
					break;
				case VEX_PREFIX_F3:
					attrMask |= ATTR_XS;
					break;
				case VEX_PREFIX_F2:
					attrMask |= ATTR_XD;
					break;
			}

			if (zFromEVEX4of4(insn->vectorExtensionPrefix[3]))
				attrMask |= ATTR_EVEXKZ;
			if (bFromEVEX4of4(insn->vectorExtensionPrefix[3]))
				attrMask |= ATTR_EVEXB;
			if (aaaFromEVEX4of4(insn->vectorExtensionPrefix[3]))
				attrMask |= ATTR_EVEXK;
			if (lFromEVEX4of4(insn->vectorExtensionPrefix[3]))
				attrMask |= ATTR_EVEXL;
			if (l2FromEVEX4of4(insn->vectorExtensionPrefix[3]))
				attrMask |= ATTR_EVEXL2;
		} else if (insn->vectorExtensionType == TYPE_VEX_3B) {
			switch (ppFromVEX3of3(insn->vectorExtensionPrefix[2])) {
				case VEX_PREFIX_66:
					attrMask |= ATTR_OPSIZE;
					break;
				case VEX_PREFIX_F3:
					attrMask |= ATTR_XS;
					break;
				case VEX_PREFIX_F2:
					attrMask |= ATTR_XD;
					break;
			}

			if (lFromVEX3of3(insn->vectorExtensionPrefix[2]))
				attrMask |= ATTR_VEXL;
		} else if (insn->vectorExtensionType == TYPE_VEX_2B) {
			switch (ppFromVEX2of2(insn->vectorExtensionPrefix[1])) {
				case VEX_PREFIX_66:
					attrMask |= ATTR_OPSIZE;
					break;
				case VEX_PREFIX_F3:
					attrMask |= ATTR_XS;
					break;
				case VEX_PREFIX_F2:
					attrMask |= ATTR_XD;
					break;
			}

			if (lFromVEX2of2(insn->vectorExtensionPrefix[1]))
				attrMask |= ATTR_VEXL;
		} else if (insn->vectorExtensionType == TYPE_XOP) {
			switch (ppFromXOP3of3(insn->vectorExtensionPrefix[2])) {
				case VEX_PREFIX_66:
					attrMask |= ATTR_OPSIZE;
					break;
				case VEX_PREFIX_F3:
					attrMask |= ATTR_XS;
					break;
				case VEX_PREFIX_F2:
					attrMask |= ATTR_XD;
					break;
			}

			if (lFromXOP3of3(insn->vectorExtensionPrefix[2]))
				attrMask |= ATTR_VEXL;
		} else {
			return -1;
		}
	} else {
		if (insn->mode != MODE_16BIT && isPrefixAtLocation(insn, 0x66, insn->necessaryPrefixLocation)) {
			attrMask |= ATTR_OPSIZE;
		} else if (isPrefixAtLocation(insn, 0x67, insn->necessaryPrefixLocation)) {
			attrMask |= ATTR_ADSIZE;
		} else if (insn->mode != MODE_16BIT && isPrefixAtLocation(insn, 0xf3, insn->necessaryPrefixLocation)) {
			attrMask |= ATTR_XS;
		} else if (insn->mode != MODE_16BIT && isPrefixAtLocation(insn, 0xf2, insn->necessaryPrefixLocation)) {
			attrMask |= ATTR_XD;
		}
	}

	if (insn->rexPrefix & 0x08)
		attrMask |= ATTR_REXW;

	/*
	 * JCXZ/JECXZ need special handling for 16-bit mode because the meaning
	 * of the AdSize prefix is inverted w.r.t. 32-bit mode.
	 */
	if (insn->mode == MODE_16BIT && insn->opcodeType == ONEBYTE &&
			insn->opcode == 0xE3)
		attrMask ^= ATTR_ADSIZE;

	if (getIDWithAttrMask(&instructionID, insn, attrMask))
		return -1;

	/* The following clauses compensate for limitations of the tables. */
	if (insn->mode != MODE_64BIT &&
			insn->vectorExtensionType != TYPE_NO_VEX_XOP) {
		/*
		 * The tables can't distinquish between cases where the W-bit is used to
		 * select register size and cases where its a required part of the opcode.
		 */
		if ((insn->vectorExtensionType == TYPE_EVEX &&
					wFromEVEX3of4(insn->vectorExtensionPrefix[2])) ||
				(insn->vectorExtensionType == TYPE_VEX_3B &&
				 wFromVEX3of3(insn->vectorExtensionPrefix[2])) ||
				(insn->vectorExtensionType == TYPE_XOP &&
				 wFromXOP3of3(insn->vectorExtensionPrefix[2]))) {
			uint16_t instructionIDWithREXW;
			if (getIDWithAttrMask(&instructionIDWithREXW,
						insn, attrMask | ATTR_REXW)) {
				insn->instructionID = instructionID;
				insn->spec = specifierForUID(instructionID);

				return 0;
			}

			// If not a 64-bit instruction. Switch the opcode.
			if (!is64Bit(instructionIDWithREXW)) {
				insn->instructionID = instructionIDWithREXW;
				insn->spec = specifierForUID(instructionIDWithREXW);

				return 0;
			}
		}
	}

	/*
	 * Absolute moves need special handling.
	 * -For 16-bit mode because the meaning of the AdSize and OpSize prefixes are
	 *  inverted w.r.t.
	 * -For 32-bit mode we need to ensure the ADSIZE prefix is observed in
	 *  any position.
	 */
	if (insn->opcodeType == ONEBYTE && ((insn->opcode & 0xFC) == 0xA0)) {
		/* Make sure we observed the prefixes in any position. */
		if (insn->isPrefix67)
			attrMask |= ATTR_ADSIZE;
		if (insn->isPrefix66)
			attrMask |= ATTR_OPSIZE;

		/* In 16-bit, invert the attributes. */
		if (insn->mode == MODE_16BIT)
			attrMask ^= ATTR_ADSIZE | ATTR_OPSIZE;

		if (getIDWithAttrMask(&instructionID, insn, attrMask))
			return -1;

		insn->instructionID = instructionID;
		insn->spec = specifierForUID(instructionID);

		return 0;
	}

	if ((insn->mode == MODE_16BIT || insn->isPrefix66) &&
			!(attrMask & ATTR_OPSIZE)) {
		/*
		 * The instruction tables make no distinction between instructions that
		 * allow OpSize anywhere (i.e., 16-bit operations) and that need it in a
		 * particular spot (i.e., many MMX operations).  In general we're
		 * conservative, but in the specific case where OpSize is present but not
		 * in the right place we check if there's a 16-bit operation.
		 */

		const struct InstructionSpecifier *spec;
		uint16_t instructionIDWithOpsize;

		spec = specifierForUID(instructionID);

		if (getIDWithAttrMask(&instructionIDWithOpsize,
					insn, attrMask | ATTR_OPSIZE)) {
			/*
			 * ModRM required with OpSize but not present; give up and return version
			 * without OpSize set
			 */

			insn->instructionID = instructionID;
			insn->spec = spec;
			return 0;
		}

		if (is16BitEquivalent(instructionID, instructionIDWithOpsize) &&
				(insn->mode == MODE_16BIT) ^ insn->isPrefix66) {
			insn->instructionID = instructionIDWithOpsize;
			insn->spec = specifierForUID(instructionIDWithOpsize);
		} else {
			insn->instructionID = instructionID;
			insn->spec = spec;
		}
		return 0;
	}

	if (insn->opcodeType == ONEBYTE && insn->opcode == 0x90 &&
			insn->rexPrefix & 0x01) {
		/*
		 * NOOP shouldn't decode as NOOP if REX.b is set. Instead
		 * it should decode as XCHG %r8, %eax.
		 */

		const struct InstructionSpecifier *spec;
		uint16_t instructionIDWithNewOpcode;
		const struct InstructionSpecifier *specWithNewOpcode;

		spec = specifierForUID(instructionID);

		/* Borrow opcode from one of the other XCHGar opcodes */
		insn->opcode = 0x91;

		if (getIDWithAttrMask(&instructionIDWithNewOpcode,
					insn,
					attrMask)) {
			insn->opcode = 0x90;

			insn->instructionID = instructionID;
			insn->spec = spec;
			return 0;
		}

		specWithNewOpcode = specifierForUID(instructionIDWithNewOpcode);

		/* Change back */
		insn->opcode = 0x90;

		insn->instructionID = instructionIDWithNewOpcode;
		insn->spec = specWithNewOpcode;

		return 0;
	}

	insn->instructionID = instructionID;
	insn->spec = specifierForUID(insn->instructionID);

	return 0;
}

/*
 * readSIB - Consumes the SIB byte to determine addressing information for an
 *   instruction.
 *
 * @param insn  - The instruction whose SIB byte is to be read.
 * @return      - 0 if the SIB byte was successfully read; nonzero otherwise.
 */
static int readSIB(struct InternalInstruction *insn)
{
	SIBIndex sibIndexBase = SIB_INDEX_NONE;
	SIBBase sibBaseBase = SIB_BASE_NONE;
	uint8_t index, base;

	// dbgprintf(insn, "readSIB()");

	if (insn->consumedSIB)
		return 0;

	insn->consumedSIB = true;

	switch (insn->addressSize) {
		case 2:
			// dbgprintf(insn, "SIB-based addressing doesn't work in 16-bit mode");
			return -1;
		case 4:
			sibIndexBase = SIB_INDEX_EAX;
			sibBaseBase = SIB_BASE_EAX;
			break;
		case 8:
			sibIndexBase = SIB_INDEX_RAX;
			sibBaseBase = SIB_BASE_RAX;
			break;
	}

	if (consumeByte(insn, &insn->sib))
		return -1;

	index = indexFromSIB(insn->sib) | (xFromREX(insn->rexPrefix) << 3);
	if (insn->vectorExtensionType == TYPE_EVEX)
		index |= v2FromEVEX4of4(insn->vectorExtensionPrefix[3]) << 4;

	switch (index) {
		case 0x4:
			insn->sibIndex = SIB_INDEX_NONE;
			break;
		default:
			insn->sibIndex = (SIBIndex)(sibIndexBase + index);
			if (insn->sibIndex == SIB_INDEX_sib ||
					insn->sibIndex == SIB_INDEX_sib64)
				insn->sibIndex = SIB_INDEX_NONE;
			break;
	}

	switch (scaleFromSIB(insn->sib)) {
		case 0:
			insn->sibScale = 1;
			break;
		case 1:
			insn->sibScale = 2;
			break;
		case 2:
			insn->sibScale = 4;
			break;
		case 3:
			insn->sibScale = 8;
			break;
	}

	base = baseFromSIB(insn->sib) | (bFromREX(insn->rexPrefix) << 3);

	switch (base) {
		case 0x5:
		case 0xd:
			switch (modFromModRM(insn->modRM)) {
				case 0x0:
					insn->eaDisplacement = EA_DISP_32;
					insn->sibBase = SIB_BASE_NONE;
					break;
				case 0x1:
					insn->eaDisplacement = EA_DISP_8;
					insn->sibBase = (SIBBase)(sibBaseBase + base);
					break;
				case 0x2:
					insn->eaDisplacement = EA_DISP_32;
					insn->sibBase = (SIBBase)(sibBaseBase + base);
					break;
				case 0x3:
					//debug("Cannot have Mod = 0b11 and a SIB byte");
					return -1;
			}
			break;
		default:
			insn->sibBase = (SIBBase)(sibBaseBase + base);
			break;
	}

	return 0;
}

/*
 * readDisplacement - Consumes the displacement of an instruction.
 *
 * @param insn  - The instruction whose displacement is to be read.
 * @return      - 0 if the displacement byte was successfully read; nonzero
 *                otherwise.
 */
static int readDisplacement(struct InternalInstruction *insn)
{
	int8_t d8;
	int16_t d16;
	int32_t d32;

	// dbgprintf(insn, "readDisplacement()");

	if (insn->consumedDisplacement)
		return 0;

	insn->consumedDisplacement = true;
	insn->displacementOffset = (uint8_t)(insn->readerCursor - insn->startLocation);

	switch (insn->eaDisplacement) {
		case EA_DISP_NONE:
			insn->consumedDisplacement = false;
			break;
		case EA_DISP_8:
			if (consumeInt8(insn, &d8))
				return -1;
			insn->displacement = d8;
			break;
		case EA_DISP_16:
			if (consumeInt16(insn, &d16))
				return -1;
			insn->displacement = d16;
			break;
		case EA_DISP_32:
			if (consumeInt32(insn, &d32))
				return -1;
			insn->displacement = d32;
			break;
	}

	return 0;
}

/*
 * readModRM - Consumes all addressing information (ModR/M byte, SIB byte, and
 *   displacement) for an instruction and interprets it.
 *
 * @param insn  - The instruction whose addressing information is to be read.
 * @return      - 0 if the information was successfully read; nonzero otherwise.
 */
static int readModRM(struct InternalInstruction *insn)
{
	uint8_t mod, rm, reg;

	// dbgprintf(insn, "readModRM()");

	// already got ModRM byte?
	if (insn->consumedModRM)
		return 0;

	insn->modRMOffset = (uint8_t)(insn->readerCursor - insn->startLocation);

	if (consumeByte(insn, &insn->modRM))
		return -1;

	// mark that we already got ModRM
	insn->consumedModRM = true;

	// save original ModRM for later reference
	insn->orgModRM = insn->modRM;

	// handle MOVcr, MOVdr, MOVrc, MOVrd by pretending they have MRM.mod = 3
	if ((insn->firstByte == 0x0f && insn->opcodeType == TWOBYTE) &&
			(insn->opcode >= 0x20 && insn->opcode <= 0x23 ))
		insn->modRM |= 0xC0;

	mod     = modFromModRM(insn->modRM);
	rm      = rmFromModRM(insn->modRM);
	reg     = regFromModRM(insn->modRM);

	/*
	 * This goes by insn->registerSize to pick the correct register, which messes
	 * up if we're using (say) XMM or 8-bit register operands.  That gets fixed in
	 * fixupReg().
	 */
	switch (insn->registerSize) {
		case 2:
			insn->regBase = MODRM_REG_AX;
			insn->eaRegBase = EA_REG_AX;
			break;
		case 4:
			insn->regBase = MODRM_REG_EAX;
			insn->eaRegBase = EA_REG_EAX;
			break;
		case 8:
			insn->regBase = MODRM_REG_RAX;
			insn->eaRegBase = EA_REG_RAX;
			break;
	}

	reg |= rFromREX(insn->rexPrefix) << 3;
	rm  |= bFromREX(insn->rexPrefix) << 3;
	if (insn->vectorExtensionType == TYPE_EVEX) {
		reg |= r2FromEVEX2of4(insn->vectorExtensionPrefix[1]) << 4;
		rm  |=  xFromEVEX2of4(insn->vectorExtensionPrefix[1]) << 4;
	}

	insn->reg = (Reg)(insn->regBase + reg);

	switch (insn->addressSize) {
		case 2:
			insn->eaBaseBase = EA_BASE_BX_SI;

			switch (mod) {
				case 0x0:
					if (rm == 0x6) {
						insn->eaBase = EA_BASE_NONE;
						insn->eaDisplacement = EA_DISP_16;
						if (readDisplacement(insn))
							return -1;
					} else {
						insn->eaBase = (EABase)(insn->eaBaseBase + rm);
						insn->eaDisplacement = EA_DISP_NONE;
					}
					break;
				case 0x1:
					insn->eaBase = (EABase)(insn->eaBaseBase + rm);
					insn->eaDisplacement = EA_DISP_8;
					insn->displacementSize = 1;
					if (readDisplacement(insn))
						return -1;
					break;
				case 0x2:
					insn->eaBase = (EABase)(insn->eaBaseBase + rm);
					insn->eaDisplacement = EA_DISP_16;
					if (readDisplacement(insn))
						return -1;
					break;
				case 0x3:
					insn->eaBase = (EABase)(insn->eaRegBase + rm);
					insn->eaDisplacement = EA_DISP_NONE;
					if (readDisplacement(insn))
						return -1;
					break;
			}
			break;
		case 4:
		case 8:
			insn->eaBaseBase = (insn->addressSize == 4 ? EA_BASE_EAX : EA_BASE_RAX);

			switch (mod) {
				case 0x0:
					insn->eaDisplacement = EA_DISP_NONE; /* readSIB may override this */
					switch (rm) {
						case 0x14:
						case 0x4:
						case 0xc:   /* in case REXW.b is set */
							insn->eaBase = (insn->addressSize == 4 ?
									EA_BASE_sib : EA_BASE_sib64);
							if (readSIB(insn) || readDisplacement(insn))
								return -1;
							break;
						case 0x5:
						case 0xd:
							insn->eaBase = EA_BASE_NONE;
							insn->eaDisplacement = EA_DISP_32;
							if (readDisplacement(insn))
								return -1;
							break;
						default:
							insn->eaBase = (EABase)(insn->eaBaseBase + rm);
							break;
					}

					break;
				case 0x1:
					insn->displacementSize = 1;
					/* FALLTHROUGH */
				case 0x2:
					insn->eaDisplacement = (mod == 0x1 ? EA_DISP_8 : EA_DISP_32);
					switch (rm) {
						case 0x14:
						case 0x4:
						case 0xc:   /* in case REXW.b is set */
							insn->eaBase = EA_BASE_sib;
							if (readSIB(insn) || readDisplacement(insn))
								return -1;
							break;
						default:
							insn->eaBase = (EABase)(insn->eaBaseBase + rm);
							if (readDisplacement(insn))
								return -1;
							break;
					}
					break;
				case 0x3:
					insn->eaDisplacement = EA_DISP_NONE;
					insn->eaBase = (EABase)(insn->eaRegBase + rm);
					break;
			}
			break;
	} /* switch (insn->addressSize) */

	return 0;
}

#define GENERIC_FIXUP_FUNC(name, base, prefix)            \
  static uint8_t name(struct InternalInstruction *insn,   \
                      OperandType type,                   \
                      uint8_t index,                      \
                      uint8_t *valid) {                   \
    *valid = 1;                                           \
    switch (type) {                                       \
    default:                                              \
      *valid = 0;                                         \
      return 0;                                           \
    case TYPE_Rv:                                         \
      return base + index;                                \
    case TYPE_R8:                                         \
      if (insn->rexPrefix &&                              \
         index >= 4 && index <= 7) {                      \
        return prefix##_SPL + (index - 4);                \
      } else {                                            \
        return prefix##_AL + index;                       \
      }                                                   \
    case TYPE_R16:                                        \
      return prefix##_AX + index;                         \
    case TYPE_R32:                                        \
      return prefix##_EAX + index;                        \
    case TYPE_R64:                                        \
      return prefix##_RAX + index;                        \
    case TYPE_XMM512:                                     \
      return prefix##_ZMM0 + index;                       \
    case TYPE_XMM256:                                     \
      return prefix##_YMM0 + index;                       \
    case TYPE_XMM128:                                     \
    case TYPE_XMM64:                                      \
    case TYPE_XMM32:                                      \
    case TYPE_XMM:                                        \
      return prefix##_XMM0 + index;                       \
    case TYPE_VK1:                                        \
    case TYPE_VK8:                                        \
    case TYPE_VK16:                                       \
      if (index > 7)                                      \
        *valid = 0;                                       \
      return prefix##_K0 + index;                         \
    case TYPE_MM64:                                       \
      return prefix##_MM0 + (index & 0x7);                \
    case TYPE_SEGMENTREG:                                 \
      if (index > 5)                                      \
        *valid = 0;                                       \
      return prefix##_ES + index;                         \
    case TYPE_DEBUGREG:                                   \
      return prefix##_DR0 + index;                        \
    case TYPE_CONTROLREG:                                 \
      return prefix##_CR0 + index;                        \
    }                                                     \
  }


/*
 * fixup*Value - Consults an operand type to determine the meaning of the
 *   reg or R/M field.  If the operand is an XMM operand, for example, an
 *   operand would be XMM0 instead of AX, which readModRM() would otherwise
 *   misinterpret it as.
 *
 * @param insn  - The instruction containing the operand.
 * @param type  - The operand type.
 * @param index - The existing value of the field as reported by readModRM().
 * @param valid - The address of a uint8_t.  The target is set to 1 if the
 *                field is valid for the register class; 0 if not.
 * @return      - The proper value.
 */
GENERIC_FIXUP_FUNC(fixupRegValue, insn->regBase,    MODRM_REG)
GENERIC_FIXUP_FUNC(fixupRMValue,  insn->eaRegBase,  EA_REG)

/*
 * fixupReg - Consults an operand specifier to determine which of the
 *   fixup*Value functions to use in correcting readModRM()'ss interpretation.
 *
 * @param insn  - See fixup*Value().
 * @param op    - The operand specifier.
 * @return      - 0 if fixup was successful; -1 if the register returned was
 *                invalid for its class.
 */
static int fixupReg(struct InternalInstruction *insn,
		const struct OperandSpecifier *op)
{
	uint8_t valid;

	// dbgprintf(insn, "fixupReg()");

	switch ((OperandEncoding)op->encoding) {
		default:
			//debug("Expected a REG or R/M encoding in fixupReg");
			return -1;
		case ENCODING_VVVV:
			insn->vvvv = (Reg)fixupRegValue(insn,
					(OperandType)op->type,
					insn->vvvv,
					&valid);
			if (!valid)
				return -1;
			break;
		case ENCODING_REG:
			insn->reg = (Reg)fixupRegValue(insn,
					(OperandType)op->type,
					(uint8_t)(insn->reg - insn->regBase),
					&valid);
			if (!valid)
				return -1;
			break;
		CASE_ENCODING_RM:
			if (insn->eaBase >= insn->eaRegBase) {
				insn->eaBase = (EABase)fixupRMValue(insn,
						(OperandType)op->type,
						(uint8_t)(insn->eaBase - insn->eaRegBase),
						&valid);
				if (!valid)
					return -1;
			}
			break;
	}

	return 0;
}

/*
 * readOpcodeRegister - Reads an operand from the opcode field of an
 *   instruction and interprets it appropriately given the operand width.
 *   Handles AddRegFrm instructions.
 *
 * @param insn  - the instruction whose opcode field is to be read.
 * @param size  - The width (in bytes) of the register being specified.
 *                1 means AL and friends, 2 means AX, 4 means EAX, and 8 means
 *                RAX.
 * @return      - 0 on success; nonzero otherwise.
 */
static int readOpcodeRegister(struct InternalInstruction *insn, uint8_t size)
{
	// dbgprintf(insn, "readOpcodeRegister()");

	if (size == 0)
		size = insn->registerSize;

	insn->operandSize = size;

	switch (size) {
		case 1:
			insn->opcodeRegister = (Reg)(MODRM_REG_AL + ((bFromREX(insn->rexPrefix) << 3)
						| (insn->opcode & 7)));
			if (insn->rexPrefix &&
					insn->opcodeRegister >= MODRM_REG_AL + 0x4 &&
					insn->opcodeRegister < MODRM_REG_AL + 0x8) {
				insn->opcodeRegister = (Reg)(MODRM_REG_SPL
						+ (insn->opcodeRegister - MODRM_REG_AL - 4));
			}

			break;
		case 2:
			insn->opcodeRegister = (Reg)(MODRM_REG_AX
					+ ((bFromREX(insn->rexPrefix) << 3)
						| (insn->opcode & 7)));
			break;
		case 4:
			insn->opcodeRegister = (Reg)(MODRM_REG_EAX
					+ ((bFromREX(insn->rexPrefix) << 3)
						| (insn->opcode & 7)));
			break;
		case 8:
			insn->opcodeRegister = (Reg)(MODRM_REG_RAX
					+ ((bFromREX(insn->rexPrefix) << 3)
						| (insn->opcode & 7)));
			break;
	}

	return 0;
}

/*
 * readImmediate - Consumes an immediate operand from an instruction, given the
 *   desired operand size.
 *
 * @param insn  - The instruction whose operand is to be read.
 * @param size  - The width (in bytes) of the operand.
 * @return      - 0 if the immediate was successfully consumed; nonzero
 *                otherwise.
 */
static int readImmediate(struct InternalInstruction *insn, uint8_t size)
{
	uint8_t imm8;
	uint16_t imm16;
	uint32_t imm32;
	uint64_t imm64;

	// dbgprintf(insn, "readImmediate()");

	if (insn->numImmediatesConsumed == 2) {
		//debug("Already consumed two immediates");
		return -1;
	}

	if (size == 0)
		size = insn->immediateSize;
	else
		insn->immediateSize = size;
	insn->immediateOffset = (uint8_t)(insn->readerCursor - insn->startLocation);

	switch (size) {
		case 1:
			if (consumeByte(insn, &imm8))
				return -1;
			insn->immediates[insn->numImmediatesConsumed] = imm8;
			break;
		case 2:
			if (consumeUInt16(insn, &imm16))
				return -1;
			insn->immediates[insn->numImmediatesConsumed] = imm16;
			break;
		case 4:
			if (consumeUInt32(insn, &imm32))
				return -1;
			insn->immediates[insn->numImmediatesConsumed] = imm32;
			break;
		case 8:
			if (consumeUInt64(insn, &imm64))
				return -1;
			insn->immediates[insn->numImmediatesConsumed] = imm64;
			break;
	}

	insn->numImmediatesConsumed++;

	return 0;
}

/*
 * readVVVV - Consumes vvvv from an instruction if it has a VEX prefix.
 *
 * @param insn  - The instruction whose operand is to be read.
 * @return      - 0 if the vvvv was successfully consumed; nonzero
 *                otherwise.
 */
static int readVVVV(struct InternalInstruction *insn)
{
	int vvvv;
	// dbgprintf(insn, "readVVVV()");

	if (insn->vectorExtensionType == TYPE_EVEX)
		vvvv = (v2FromEVEX4of4(insn->vectorExtensionPrefix[3]) << 4 |
				vvvvFromEVEX3of4(insn->vectorExtensionPrefix[2]));
	else if (insn->vectorExtensionType == TYPE_VEX_3B)
		vvvv = vvvvFromVEX3of3(insn->vectorExtensionPrefix[2]);
	else if (insn->vectorExtensionType == TYPE_VEX_2B)
		vvvv = vvvvFromVEX2of2(insn->vectorExtensionPrefix[1]);
	else if (insn->vectorExtensionType == TYPE_XOP)
		vvvv = vvvvFromXOP3of3(insn->vectorExtensionPrefix[2]);
	else
		return -1;

	if (insn->mode != MODE_64BIT)
		vvvv &= 0x7;

	insn->vvvv = vvvv;

	return 0;
}

/*
 * readMaskRegister - Reads an mask register from the opcode field of an
 *   instruction.
 *
 * @param insn    - The instruction whose opcode field is to be read.
 * @return        - 0 on success; nonzero otherwise.
 */
static int readMaskRegister(struct InternalInstruction *insn)
{
	// dbgprintf(insn, "readMaskRegister()");

	if (insn->vectorExtensionType != TYPE_EVEX)
		return -1;

	insn->writemask = aaaFromEVEX4of4(insn->vectorExtensionPrefix[3]);

	return 0;
}

/*
 * readOperands - Consults the specifier for an instruction and consumes all
 *   operands for that instruction, interpreting them as it goes.
 *
 * @param insn  - The instruction whose operands are to be read and interpreted.
 * @return      - 0 if all operands could be read; nonzero otherwise.
 */
static int readOperands(struct InternalInstruction *insn)
{
	int index;
	int hasVVVV, needVVVV;
	int sawRegImm = 0;

	// printf(">>> readOperands(): ID = %u\n", insn->instructionID);
	/* If non-zero vvvv specified, need to make sure one of the operands
	   uses it. */
	hasVVVV = !readVVVV(insn);
	needVVVV = hasVVVV && (insn->vvvv != 0);

	for (index = 0; index < X86_MAX_OPERANDS; ++index) {
		//printf(">>> encoding[%u] = %u\n", index, x86OperandSets[insn->spec->operands][index].encoding);
		switch (x86OperandSets[insn->spec->operands][index].encoding) {
			case ENCODING_NONE:
			case ENCODING_SI:
			case ENCODING_DI:
				break;
			case ENCODING_REG:
			CASE_ENCODING_RM:
				if (readModRM(insn))
					return -1;
				if (fixupReg(insn, &x86OperandSets[insn->spec->operands][index]))
					return -1;
				// Apply the AVX512 compressed displacement scaling factor.
				if (x86OperandSets[insn->spec->operands][index].encoding != ENCODING_REG && insn->eaDisplacement == EA_DISP_8)
					insn->displacement *= (int64_t)1 << (x86OperandSets[insn->spec->operands][index].encoding - ENCODING_RM);
				break;
			case ENCODING_CB:
			case ENCODING_CW:
			case ENCODING_CD:
			case ENCODING_CP:
			case ENCODING_CO:
			case ENCODING_CT:
				// dbgprintf(insn, "We currently don't hande code-offset encodings");
				return -1;
			case ENCODING_IB:
				if (sawRegImm) {
					/* Saw a register immediate so don't read again and instead split the
					   previous immediate.  FIXME: This is a hack. */
					insn->immediates[insn->numImmediatesConsumed] =
						insn->immediates[insn->numImmediatesConsumed - 1] & 0xf;
					++insn->numImmediatesConsumed;
					break;
				}
				if (readImmediate(insn, 1))
					return -1;
				if (x86OperandSets[insn->spec->operands][index].type == TYPE_XMM128 ||
						x86OperandSets[insn->spec->operands][index].type == TYPE_XMM256)
					sawRegImm = 1;
				break;
			case ENCODING_IW:
				if (readImmediate(insn, 2))
					return -1;
				break;
			case ENCODING_ID:
				if (readImmediate(insn, 4))
					return -1;
				break;
			case ENCODING_IO:
				if (readImmediate(insn, 8))
					return -1;
				break;
			case ENCODING_Iv:
				if (readImmediate(insn, insn->immediateSize))
					return -1;
				break;
			case ENCODING_Ia:
				if (readImmediate(insn, insn->addressSize))
					return -1;
				/* Direct memory-offset (moffset) immediate will get mapped
				   to memory operand later. We want the encoding info to
				   reflect that as well. */
				insn->displacementOffset = insn->immediateOffset;
				insn->consumedDisplacement = true;
				insn->displacementSize = insn->immediateSize;
				insn->displacement = insn->immediates[insn->numImmediatesConsumed - 1];
				insn->immediateOffset = 0;
				insn->immediateSize = 0;
				break;
			case ENCODING_RB:
				if (readOpcodeRegister(insn, 1))
					return -1;
				break;
			case ENCODING_RW:
				if (readOpcodeRegister(insn, 2))
					return -1;
				break;
			case ENCODING_RD:
				if (readOpcodeRegister(insn, 4))
					return -1;
				break;
			case ENCODING_RO:
				if (readOpcodeRegister(insn, 8))
					return -1;
				break;
			case ENCODING_Rv:
				if (readOpcodeRegister(insn, 0))
					return -1;
				break;
			case ENCODING_FP:
				break;
			case ENCODING_VVVV:
				needVVVV = 0; /* Mark that we have found a VVVV operand. */
				if (!hasVVVV)
					return -1;
				if (fixupReg(insn, &x86OperandSets[insn->spec->operands][index]))
					return -1;
				break;
			case ENCODING_WRITEMASK:
				if (readMaskRegister(insn))
					return -1;
				break;
			case ENCODING_DUP:
				break;
			default:
				// dbgprintf(insn, "Encountered an operand with an unknown encoding.");
				return -1;
		}
	}

	/* If we didn't find ENCODING_VVVV operand, but non-zero vvvv present, fail */
	if (needVVVV) return -1;

	return 0;
}

// return True if instruction is illegal to use with prefixes
// This also check & fix the isPrefixNN when a prefix is irrelevant.
static bool checkPrefix(struct InternalInstruction *insn)
{
	// LOCK prefix
	if (insn->isPrefixf0) {
		switch(insn->instructionID) {
			default:
				// invalid LOCK
				return true;

			// nop dword [rax]
			case X86_NOOPL:

			// DEC
			case X86_DEC16m:
			case X86_DEC32m:
			case X86_DEC64m:
			case X86_DEC8m:

			// ADC
			case X86_ADC16mi:
			case X86_ADC16mi8:
			case X86_ADC16mr:
			case X86_ADC32mi:
			case X86_ADC32mi8:
			case X86_ADC32mr:
			case X86_ADC64mi32:
			case X86_ADC64mi8:
			case X86_ADC64mr:
			case X86_ADC8mi:
			case X86_ADC8mi8:
			case X86_ADC8mr:
			case X86_ADC8rm:
			case X86_ADC16rm:
			case X86_ADC32rm:
			case X86_ADC64rm:

			// ADD
			case X86_ADD16mi:
			case X86_ADD16mi8:
			case X86_ADD16mr:
			case X86_ADD32mi:
			case X86_ADD32mi8:
			case X86_ADD32mr:
			case X86_ADD64mi32:
			case X86_ADD64mi8:
			case X86_ADD64mr:
			case X86_ADD8mi:
			case X86_ADD8mi8:
			case X86_ADD8mr:
			case X86_ADD8rm:
			case X86_ADD16rm:
			case X86_ADD32rm:
			case X86_ADD64rm:

			// AND
			case X86_AND16mi:
			case X86_AND16mi8:
			case X86_AND16mr:
			case X86_AND32mi:
			case X86_AND32mi8:
			case X86_AND32mr:
			case X86_AND64mi32:
			case X86_AND64mi8:
			case X86_AND64mr:
			case X86_AND8mi:
			case X86_AND8mi8:
			case X86_AND8mr:
			case X86_AND8rm:
			case X86_AND16rm:
			case X86_AND32rm:
			case X86_AND64rm:


			// BTC
			case X86_BTC16mi8:
			case X86_BTC16mr:
			case X86_BTC32mi8:
			case X86_BTC32mr:
			case X86_BTC64mi8:
			case X86_BTC64mr:

			// BTR
			case X86_BTR16mi8:
			case X86_BTR16mr:
			case X86_BTR32mi8:
			case X86_BTR32mr:
			case X86_BTR64mi8:
			case X86_BTR64mr:

			// BTS
			case X86_BTS16mi8:
			case X86_BTS16mr:
			case X86_BTS32mi8:
			case X86_BTS32mr:
			case X86_BTS64mi8:
			case X86_BTS64mr:

			// CMPXCHG
			case X86_CMPXCHG16B:
			case X86_CMPXCHG16rm:
			case X86_CMPXCHG32rm:
			case X86_CMPXCHG64rm:
			case X86_CMPXCHG8rm:
			case X86_CMPXCHG8B:

			// INC
			case X86_INC16m:
			case X86_INC32m:
			case X86_INC64m:
			case X86_INC8m:

			// NEG
			case X86_NEG16m:
			case X86_NEG32m:
			case X86_NEG64m:
			case X86_NEG8m:

			// NOT
			case X86_NOT16m:
			case X86_NOT32m:
			case X86_NOT64m:
			case X86_NOT8m:

			// OR
			case X86_OR16mi:
			case X86_OR16mi8:
			case X86_OR16mr:
			case X86_OR32mi:
			case X86_OR32mi8:
			case X86_OR32mr:
			case X86_OR32mrLocked:
			case X86_OR64mi32:
			case X86_OR64mi8:
			case X86_OR64mr:
			case X86_OR8mi8:
			case X86_OR8mi:
			case X86_OR8mr:
			case X86_OR8rm:
			case X86_OR16rm:
			case X86_OR32rm:
			case X86_OR64rm:

			// SBB
			case X86_SBB16mi:
			case X86_SBB16mi8:
			case X86_SBB16mr:
			case X86_SBB32mi:
			case X86_SBB32mi8:
			case X86_SBB32mr:
			case X86_SBB64mi32:
			case X86_SBB64mi8:
			case X86_SBB64mr:
			case X86_SBB8mi:
			case X86_SBB8mi8:
			case X86_SBB8mr:

			// SUB
			case X86_SUB16mi:
			case X86_SUB16mi8:
			case X86_SUB16mr:
			case X86_SUB32mi:
			case X86_SUB32mi8:
			case X86_SUB32mr:
			case X86_SUB64mi32:
			case X86_SUB64mi8:
			case X86_SUB64mr:
			case X86_SUB8mi8:
			case X86_SUB8mi:
			case X86_SUB8mr:
			case X86_SUB8rm:
			case X86_SUB16rm:
			case X86_SUB32rm:
			case X86_SUB64rm:

			// XADD
			case X86_XADD16rm:
			case X86_XADD32rm:
			case X86_XADD64rm:
			case X86_XADD8rm:

			// XCHG
			case X86_XCHG16rm:
			case X86_XCHG32rm:
			case X86_XCHG64rm:
			case X86_XCHG8rm:

			// XOR
			case X86_XOR16mi:
			case X86_XOR16mi8:
			case X86_XOR16mr:
			case X86_XOR32mi:
			case X86_XOR32mi8:
			case X86_XOR32mr:
			case X86_XOR64mi32:
			case X86_XOR64mi8:
			case X86_XOR64mr:
			case X86_XOR8mi8:
			case X86_XOR8mi:
			case X86_XOR8mr:
			case X86_XOR8rm:
			case X86_XOR16rm:
			case X86_XOR32rm:
			case X86_XOR64rm:

				// this instruction can be used with LOCK prefix
				return false;
		}
	}

	// REPNE prefix
	if (insn->isPrefixf2) {
		// 0xf2 can be a part of instruction encoding, but not really a prefix.
		// In such a case, clear it.
		if (insn->twoByteEscape == 0x0f) {
			insn->prefix0 = 0;
		}
	}

	// no invalid prefixes
	return false;
}

/*
 * decodeInstruction - Reads and interprets a full instruction provided by the
 *   user.
 *
 * @param insn      - A pointer to the instruction to be populated.  Must be
 *                    pre-allocated.
 * @param reader    - The function to be used to read the instruction's bytes.
 * @param readerArg - A generic argument to be passed to the reader to store
 *                    any internal state.
 * @param startLoc  - The address (in the reader's address space) of the first
 *                    byte in the instruction.
 * @param mode      - The mode (real mode, IA-32e, or IA-32e in 64-bit mode) to
 *                    decode the instruction in.
 * @return          - 0 if instruction is valid; nonzero if not.
 */
int decodeInstruction(struct InternalInstruction *insn,
		byteReader_t reader,
		const void *readerArg,
		uint64_t startLoc,
		DisassemblerMode mode)
{
	insn->reader = reader;
	insn->readerArg = readerArg;
	insn->startLocation = startLoc;
	insn->readerCursor = startLoc;
	insn->mode = mode;

	if (readPrefixes(insn)       ||
			readOpcode(insn)         ||
			getID(insn)      ||
			insn->instructionID == 0 ||
			checkPrefix(insn) ||
			readOperands(insn))
		return -1;

	insn->length = (size_t)(insn->readerCursor - insn->startLocation);

	// instruction length must be <= 15 to be valid
	if (insn->length > 15)
		return -1;

	if (insn->operandSize == 0)
		insn->operandSize = insn->registerSize;

	insn->operands = &x86OperandSets[insn->spec->operands][0];

	return 0;
}

#endif
