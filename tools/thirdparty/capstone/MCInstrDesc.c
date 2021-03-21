/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#include "MCInstrDesc.h"

/// isPredicate - Set if this is one of the operands that made up of
/// the predicate operand that controls an isPredicable() instruction.
bool MCOperandInfo_isPredicate(const MCOperandInfo *m)
{
	return m->Flags & (1 << MCOI_Predicate);
}

/// isOptionalDef - Set if this operand is a optional def.
///
bool MCOperandInfo_isOptionalDef(const MCOperandInfo *m)
{
	return m->Flags & (1 << MCOI_OptionalDef);
}
