/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_MCDISASSEMBLER_H
#define CS_MCDISASSEMBLER_H

typedef enum DecodeStatus {
	MCDisassembler_Fail = 0,
	MCDisassembler_SoftFail = 1,
	MCDisassembler_Success = 3,
} DecodeStatus;

#endif

