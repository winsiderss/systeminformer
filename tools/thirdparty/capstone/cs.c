/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */
#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#pragma warning(disable:4996)			// disable MSVC's warning on strcpy()
#pragma warning(disable:28719)		// disable MSVC's warning on strcpy()
#endif
#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#include <libkern/libkern.h>
#else
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include <string.h>
#include "../capstone/include/capstone/capstone.h"

#include "utils.h"
#include "MCRegisterInfo.h"

#if defined(_KERNEL_MODE)
#include "windows\winkernel_mm.h"
#endif

// Issue #681: Windows kernel does not support formatting float point
#if defined(_KERNEL_MODE) && !defined(CAPSTONE_DIET)
#if defined(CAPSTONE_HAS_ARM) || defined(CAPSTONE_HAS_ARM64) || defined(CAPSTONE_HAS_M68K)
#define CAPSTONE_STR_INTERNAL(x) #x
#define CAPSTONE_STR(x) CAPSTONE_STR_INTERNAL(x)
#define CAPSTONE_MSVC_WRANING_PREFIX __FILE__ "("CAPSTONE_STR(__LINE__)") : warning message : "

#pragma message(CAPSTONE_MSVC_WRANING_PREFIX "Windows driver does not support full features for selected architecture(s). Define CAPSTONE_DIET to compile Capstone with only supported features. See issue #681 for details.")

#undef CAPSTONE_MSVC_WRANING_PREFIX
#undef CAPSTONE_STR
#undef CAPSTONE_STR_INTERNAL
#endif
#endif	// defined(_KERNEL_MODE) && !defined(CAPSTONE_DIET)

#if !defined(CAPSTONE_HAS_OSXKERNEL) && !defined(CAPSTONE_DIET) && !defined(_KERNEL_MODE)
#define INSN_CACHE_SIZE 32
#else
// reduce stack variable size for kernel/firmware
#define INSN_CACHE_SIZE 8
#endif

// default SKIPDATA mnemonic
#ifndef CAPSTONE_DIET
#define SKIPDATA_MNEM ".byte"
#else // No printing is available in diet mode
#define SKIPDATA_MNEM NULL
#endif

#include "arch/AArch64/AArch64Module.h"
#include "arch/ARM/ARMModule.h"
#include "arch/EVM/EVMModule.h"
#include "arch/M680X/M680XModule.h"
#include "arch/M68K/M68KModule.h"
#include "arch/Mips/MipsModule.h"
#include "arch/PowerPC/PPCModule.h"
#include "arch/Sparc/SparcModule.h"
#include "arch/SystemZ/SystemZModule.h"
#include "arch/TMS320C64x/TMS320C64xModule.h"
#include "arch/X86/X86Module.h"
#include "arch/XCore/XCoreModule.h"

// constructor initialization for all archs
static cs_err (*cs_arch_init[MAX_ARCH])(cs_struct *) = {
#ifdef CAPSTONE_HAS_ARM
	ARM_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_ARM64
	AArch64_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_MIPS
	Mips_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_X86
	X86_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_POWERPC
	PPC_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_SPARC
	Sparc_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_SYSZ
	SystemZ_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_XCORE
	XCore_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_M68K
	M68K_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_TMS320C64X
	TMS320C64x_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_M680X
	M680X_global_init,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_EVM
	EVM_global_init,
#else
	NULL,
#endif
};

// support cs_option() for all archs
static cs_err (*cs_arch_option[MAX_ARCH]) (cs_struct *, cs_opt_type, size_t value) = {
#ifdef CAPSTONE_HAS_ARM
	ARM_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_ARM64
	AArch64_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_MIPS
	Mips_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_X86
	X86_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_POWERPC
	PPC_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_SPARC
	Sparc_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_SYSZ
	SystemZ_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_XCORE
	XCore_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_M68K
	M68K_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_TMS320C64X
	TMS320C64x_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_M680X
	M680X_option,
#else
	NULL,
#endif
#ifdef CAPSTONE_HAS_EVM
	EVM_option,
#else
	NULL,
#endif
};

// bitmask for finding disallowed modes for an arch:
// to be called in cs_open()/cs_option()
static cs_mode cs_arch_disallowed_mode_mask[MAX_ARCH] = {
#ifdef CAPSTONE_HAS_ARM
	~(CS_MODE_LITTLE_ENDIAN | CS_MODE_ARM | CS_MODE_V8 | CS_MODE_MCLASS
	  | CS_MODE_THUMB | CS_MODE_BIG_ENDIAN),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_ARM64
	~(CS_MODE_LITTLE_ENDIAN | CS_MODE_ARM | CS_MODE_BIG_ENDIAN),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_MIPS
	~(CS_MODE_LITTLE_ENDIAN | CS_MODE_32 | CS_MODE_64 | CS_MODE_MICRO
	  | CS_MODE_MIPS32R6 | CS_MODE_BIG_ENDIAN | CS_MODE_MIPS2 | CS_MODE_MIPS3),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_X86
	~(CS_MODE_LITTLE_ENDIAN | CS_MODE_32 | CS_MODE_64 | CS_MODE_16),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_POWERPC
	~(CS_MODE_LITTLE_ENDIAN | CS_MODE_32 | CS_MODE_64 | CS_MODE_BIG_ENDIAN
	  | CS_MODE_QPX),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_SPARC
	~(CS_MODE_BIG_ENDIAN | CS_MODE_V9),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_SYSZ
	~(CS_MODE_BIG_ENDIAN),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_XCORE
	~(CS_MODE_BIG_ENDIAN),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_M68K
	~(CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000 | CS_MODE_M68K_010 | CS_MODE_M68K_020
	  | CS_MODE_M68K_030 | CS_MODE_M68K_040 | CS_MODE_M68K_060),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_TMS320C64X
	~(CS_MODE_BIG_ENDIAN),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_M680X
	~(CS_MODE_M680X_6301 | CS_MODE_M680X_6309 | CS_MODE_M680X_6800
	  | CS_MODE_M680X_6801 | CS_MODE_M680X_6805 | CS_MODE_M680X_6808
	  | CS_MODE_M680X_6809 | CS_MODE_M680X_6811 | CS_MODE_M680X_CPU12
	  | CS_MODE_M680X_HCS08),
#else
	0,
#endif
#ifdef CAPSTONE_HAS_EVM
	0,
#else
	0,
#endif
};

// bitmask of enabled architectures
static uint32_t all_arch = 0
#ifdef CAPSTONE_HAS_ARM
	| (1 << CS_ARCH_ARM)
#endif
#ifdef CAPSTONE_HAS_ARM64
	| (1 << CS_ARCH_ARM64)
#endif
#ifdef CAPSTONE_HAS_MIPS
	| (1 << CS_ARCH_MIPS)
#endif
#ifdef CAPSTONE_HAS_X86
	| (1 << CS_ARCH_X86)
#endif
#ifdef CAPSTONE_HAS_POWERPC
	| (1 << CS_ARCH_PPC)
#endif
#ifdef CAPSTONE_HAS_SPARC
	| (1 << CS_ARCH_SPARC)
#endif
#ifdef CAPSTONE_HAS_SYSZ
	| (1 << CS_ARCH_SYSZ)
#endif
#ifdef CAPSTONE_HAS_XCORE
	| (1 << CS_ARCH_XCORE)
#endif
#ifdef CAPSTONE_HAS_M68K
	| (1 << CS_ARCH_M68K)
#endif
#ifdef CAPSTONE_HAS_TMS320C64X
	| (1 << CS_ARCH_TMS320C64X)
#endif
#ifdef CAPSTONE_HAS_M680X
	| (1 << CS_ARCH_M680X)
#endif
#ifdef CAPSTONE_HAS_EVM
	| (1 << CS_ARCH_EVM)
#endif
;


#if defined(CAPSTONE_USE_SYS_DYN_MEM)
#if !defined(CAPSTONE_HAS_OSXKERNEL) && !defined(_KERNEL_MODE)
// default
cs_malloc_t cs_mem_malloc = malloc;
cs_calloc_t cs_mem_calloc = calloc;
cs_realloc_t cs_mem_realloc = realloc;
cs_free_t cs_mem_free = free;
#if defined(_WIN32_WCE)
cs_vsnprintf_t cs_vsnprintf = _vsnprintf;
#else
cs_vsnprintf_t cs_vsnprintf = vsnprintf;
#endif  // defined(_WIN32_WCE)

#elif defined(_KERNEL_MODE)
// Windows driver
cs_malloc_t cs_mem_malloc = cs_winkernel_malloc;
cs_calloc_t cs_mem_calloc = cs_winkernel_calloc;
cs_realloc_t cs_mem_realloc = cs_winkernel_realloc;
cs_free_t cs_mem_free = cs_winkernel_free;
cs_vsnprintf_t cs_vsnprintf = cs_winkernel_vsnprintf;
#else
// OSX kernel
extern void* kern_os_malloc(size_t size);
extern void kern_os_free(void* addr);
extern void* kern_os_realloc(void* addr, size_t nsize);

static void* cs_kern_os_calloc(size_t num, size_t size)
{
	return kern_os_malloc(num * size); // malloc bzeroes the buffer
}

cs_malloc_t cs_mem_malloc = kern_os_malloc;
cs_calloc_t cs_mem_calloc = cs_kern_os_calloc;
cs_realloc_t cs_mem_realloc = kern_os_realloc;
cs_free_t cs_mem_free = kern_os_free;
cs_vsnprintf_t cs_vsnprintf = vsnprintf;
#endif  // !defined(CAPSTONE_HAS_OSXKERNEL) && !defined(_KERNEL_MODE)
#else
// User-defined
cs_malloc_t cs_mem_malloc = NULL;
cs_calloc_t cs_mem_calloc = NULL;
cs_realloc_t cs_mem_realloc = NULL;
cs_free_t cs_mem_free = NULL;
cs_vsnprintf_t cs_vsnprintf = NULL;

#endif  // defined(CAPSTONE_USE_SYS_DYN_MEM)

CAPSTONE_EXPORT
unsigned int CAPSTONE_API cs_version(int *major, int *minor)
{
	if (major != NULL && minor != NULL) {
		*major = CS_API_MAJOR;
		*minor = CS_API_MINOR;
	}

	return (CS_API_MAJOR << 8) + CS_API_MINOR;
}

CAPSTONE_EXPORT
bool CAPSTONE_API cs_support(int query)
{
	if (query == CS_ARCH_ALL)
		return all_arch == ((1 << CS_ARCH_ARM) | (1 << CS_ARCH_ARM64) |
				(1 << CS_ARCH_MIPS) | (1 << CS_ARCH_X86) |
				(1 << CS_ARCH_PPC) | (1 << CS_ARCH_SPARC) |
				(1 << CS_ARCH_SYSZ) | (1 << CS_ARCH_XCORE) |
				(1 << CS_ARCH_M68K) | (1 << CS_ARCH_TMS320C64X) |
				(1 << CS_ARCH_M680X) | (1 << CS_ARCH_EVM));

	if ((unsigned int)query < CS_ARCH_MAX)
		return all_arch & (1 << query);

	if (query == CS_SUPPORT_DIET) {
#ifdef CAPSTONE_DIET
		return true;
#else
		return false;
#endif
	}

	if (query == CS_SUPPORT_X86_REDUCE) {
#if defined(CAPSTONE_HAS_X86) && defined(CAPSTONE_X86_REDUCE)
		return true;
#else
		return false;
#endif
	}

	// unsupported query
	return false;
}

CAPSTONE_EXPORT
cs_err CAPSTONE_API cs_errno(csh handle)
{
	struct cs_struct *ud;
	if (!handle)
		return CS_ERR_CSH;

	ud = (struct cs_struct *)(uintptr_t)handle;

	return ud->errnum;
}

CAPSTONE_EXPORT
const char * CAPSTONE_API cs_strerror(cs_err code)
{
	switch(code) {
		default:
			return "Unknown error code";
		case CS_ERR_OK:
			return "OK (CS_ERR_OK)";
		case CS_ERR_MEM:
			return "Out of memory (CS_ERR_MEM)";
		case CS_ERR_ARCH:
			return "Invalid/unsupported architecture(CS_ERR_ARCH)";
		case CS_ERR_HANDLE:
			return "Invalid handle (CS_ERR_HANDLE)";
		case CS_ERR_CSH:
			return "Invalid csh (CS_ERR_CSH)";
		case CS_ERR_MODE:
			return "Invalid mode (CS_ERR_MODE)";
		case CS_ERR_OPTION:
			return "Invalid option (CS_ERR_OPTION)";
		case CS_ERR_DETAIL:
			return "Details are unavailable (CS_ERR_DETAIL)";
		case CS_ERR_MEMSETUP:
			return "Dynamic memory management uninitialized (CS_ERR_MEMSETUP)";
		case CS_ERR_VERSION:
			return "Different API version between core & binding (CS_ERR_VERSION)";
		case CS_ERR_DIET:
			return "Information irrelevant in diet engine (CS_ERR_DIET)";
		case CS_ERR_SKIPDATA:
			return "Information irrelevant for 'data' instruction in SKIPDATA mode (CS_ERR_SKIPDATA)";
		case CS_ERR_X86_ATT:
			return "AT&T syntax is unavailable (CS_ERR_X86_ATT)";
		case CS_ERR_X86_INTEL:
			return "INTEL syntax is unavailable (CS_ERR_X86_INTEL)";
		case CS_ERR_X86_MASM:
			return "MASM syntax is unavailable (CS_ERR_X86_MASM)";
	}
}

CAPSTONE_EXPORT
cs_err CAPSTONE_API cs_open(cs_arch arch, cs_mode mode, csh *handle)
{
	cs_err err;
	struct cs_struct *ud;
	if (!cs_mem_malloc || !cs_mem_calloc || !cs_mem_realloc || !cs_mem_free || !cs_vsnprintf)
		// Error: before cs_open(), dynamic memory management must be initialized
		// with cs_option(CS_OPT_MEM)
		return CS_ERR_MEMSETUP;

	if (arch < CS_ARCH_MAX && cs_arch_init[arch]) {
		// verify if requested mode is valid
		if (mode & cs_arch_disallowed_mode_mask[arch]) {
			*handle = 0;
			return CS_ERR_MODE;
		}

		ud = cs_mem_calloc(1, sizeof(*ud));
		if (!ud) {
			// memory insufficient
			return CS_ERR_MEM;
		}

		ud->errnum = CS_ERR_OK;
		ud->arch = arch;
		ud->mode = mode;
		// by default, do not break instruction into details
		ud->detail = CS_OPT_OFF;

		// default skipdata setup
		ud->skipdata_setup.mnemonic = SKIPDATA_MNEM;

		err = cs_arch_init[ud->arch](ud);
		if (err) {
			cs_mem_free(ud);
			*handle = 0;
			return err;
		}

		*handle = (uintptr_t)ud;

		return CS_ERR_OK;
	} else {
		*handle = 0;
		return CS_ERR_ARCH;
	}
}

CAPSTONE_EXPORT
cs_err CAPSTONE_API cs_close(csh *handle)
{
	struct cs_struct *ud;
	struct insn_mnem *next, *tmp;

	if (*handle == 0)
		// invalid handle
		return CS_ERR_CSH;

	ud = (struct cs_struct *)(*handle);

	if (ud->printer_info)
		cs_mem_free(ud->printer_info);

	// free the linked list of customized mnemonic
	tmp = ud->mnem_list;
	while(tmp) {
		next = tmp->next;
		cs_mem_free(tmp);
		tmp = next;
	}

	cs_mem_free(ud->insn_cache);

	memset(ud, 0, sizeof(*ud));
	cs_mem_free(ud);

	// invalidate this handle by ZERO out its value.
	// this is to make sure it is unusable after cs_close()
	*handle = 0;

	return CS_ERR_OK;
}

// fill insn with mnemonic & operands info
static void fill_insn(struct cs_struct *handle, cs_insn *insn, char *buffer, MCInst *mci,
		PostPrinter_t postprinter, const uint8_t *code)
{
#ifndef CAPSTONE_DIET
	char *sp, *mnem;
#endif
	uint16_t copy_size = MIN(sizeof(insn->bytes), insn->size);

	// fill the instruction bytes.
	// we might skip some redundant bytes in front in the case of X86
	memcpy(insn->bytes, code + insn->size - copy_size, copy_size);
	insn->size = copy_size;

	// alias instruction might have ID saved in OpcodePub
	if (MCInst_getOpcodePub(mci))
		insn->id = MCInst_getOpcodePub(mci);

	// post printer handles some corner cases (hacky)
	if (postprinter)
		postprinter((csh)handle, insn, buffer, mci);

#ifndef CAPSTONE_DIET
	// fill in mnemonic & operands
	// find first space or tab
	mnem = insn->mnemonic;
	for (sp = buffer; *sp; sp++) {
		if (*sp == ' '|| *sp == '\t')
			break;
		if (*sp == '|')	// lock|rep prefix for x86
			*sp = ' ';
		// copy to @mnemonic
		*mnem = *sp;
		mnem++;
	}

	*mnem = '\0';

	// we might have customized mnemonic
	if (handle->mnem_list) {
		struct insn_mnem *tmp = handle->mnem_list;
		while(tmp) {
			if (tmp->insn.id == insn->id) {
				// found this instruction, so copy its mnemonic
				(void)strncpy(insn->mnemonic, tmp->insn.mnemonic, sizeof(insn->mnemonic) - 1);
				insn->mnemonic[sizeof(insn->mnemonic) - 1] = '\0';
				break;
			}
			tmp = tmp->next;
		}
	}

	// copy @op_str
	if (*sp) {
		// find the next non-space char
		sp++;
		for (; ((*sp == ' ') || (*sp == '\t')); sp++);
		strncpy(insn->op_str, sp, sizeof(insn->op_str) - 1);
		insn->op_str[sizeof(insn->op_str) - 1] = '\0';
	} else
		insn->op_str[0] = '\0';
#endif
}

// how many bytes will we skip when encountering data (CS_OPT_SKIPDATA)?
// this very much depends on instruction alignment requirement of each arch.
static uint8_t skipdata_size(cs_struct *handle)
{
	switch(handle->arch) {
		default:
			// should never reach
			return (uint8_t)-1;
		case CS_ARCH_ARM:
			// skip 2 bytes on Thumb mode.
			if (handle->mode & CS_MODE_THUMB)
				return 2;
			// otherwise, skip 4 bytes
			return 4;
		case CS_ARCH_ARM64:
		case CS_ARCH_MIPS:
		case CS_ARCH_PPC:
		case CS_ARCH_SPARC:
			// skip 4 bytes
			return 4;
		case CS_ARCH_SYSZ:
			// SystemZ instruction's length can be 2, 4 or 6 bytes,
			// so we just skip 2 bytes
			return 2;
		case CS_ARCH_X86:
			// X86 has no restriction on instruction alignment
			return 1;
		case CS_ARCH_XCORE:
			// XCore instruction's length can be 2 or 4 bytes,
			// so we just skip 2 bytes
			return 2;
		case CS_ARCH_M68K:
			// M68K has 2 bytes instruction alignment but contain multibyte instruction so we skip 2 bytes
			return 2;
		case CS_ARCH_TMS320C64X:
			// TMS320C64x alignment is 4.
			return 4;
		case CS_ARCH_M680X:
			// M680X alignment is 1.
			return 1;
		case CS_ARCH_EVM:
			// EVM alignment is 1.
			return 1;
	}
}

CAPSTONE_EXPORT
cs_err CAPSTONE_API cs_option(csh ud, cs_opt_type type, size_t value)
{
	struct cs_struct *handle;
	cs_opt_mnem *opt;

	// cs_option() can be called with NULL handle just for CS_OPT_MEM
	// This is supposed to be executed before all other APIs (even cs_open())
	if (type == CS_OPT_MEM) {
		cs_opt_mem *mem = (cs_opt_mem *)value;

		cs_mem_malloc = mem->malloc;
		cs_mem_calloc = mem->calloc;
		cs_mem_realloc = mem->realloc;
		cs_mem_free = mem->free;
		cs_vsnprintf = mem->vsnprintf;

		return CS_ERR_OK;
	}

	handle = (struct cs_struct *)(uintptr_t)ud;
	if (!handle)
		return CS_ERR_CSH;

	switch(type) {
		default:
			break;

		case CS_OPT_UNSIGNED:
			handle->imm_unsigned = (cs_opt_value)value;
			return CS_ERR_OK;

		case CS_OPT_DETAIL:
			handle->detail = (cs_opt_value)value;
			return CS_ERR_OK;

		case CS_OPT_SKIPDATA:
			handle->skipdata = (value == CS_OPT_ON);
			if (handle->skipdata) {
				if (handle->skipdata_size == 0) {
					// set the default skipdata size
					handle->skipdata_size = skipdata_size(handle);
				}
			}
			return CS_ERR_OK;

		case CS_OPT_SKIPDATA_SETUP:
			if (value)
				handle->skipdata_setup = *((cs_opt_skipdata *)value);
			return CS_ERR_OK;

		case CS_OPT_MNEMONIC:
			opt = (cs_opt_mnem *)value;
			if (opt->id) {
				if (opt->mnemonic) {
					struct insn_mnem *tmp;

					// add new instruction, or replace existing instruction
					// 1. find if we already had this insn in the linked list
					tmp = handle->mnem_list;
					while(tmp) {
						if (tmp->insn.id == opt->id) {
							// found this instruction, so replace its mnemonic
							(void)strncpy(tmp->insn.mnemonic, opt->mnemonic, sizeof(tmp->insn.mnemonic) - 1);
							tmp->insn.mnemonic[sizeof(tmp->insn.mnemonic) - 1] = '\0';
							break;
						}
						tmp = tmp->next;
					}

					// 2. add this instruction if we have not had it yet
					if (!tmp) {
						tmp = cs_mem_malloc(sizeof(*tmp));
						tmp->insn.id = opt->id;
						(void)strncpy(tmp->insn.mnemonic, opt->mnemonic, sizeof(tmp->insn.mnemonic) - 1);
						tmp->insn.mnemonic[sizeof(tmp->insn.mnemonic) - 1] = '\0';
						// this new instruction is heading the list
						tmp->next = handle->mnem_list;
						handle->mnem_list = tmp;
					}
					return CS_ERR_OK;
				} else {
					struct insn_mnem *prev, *tmp;

					// we want to delete an existing instruction
					// iterate the list to find the instruction to remove it
					tmp = handle->mnem_list;
					prev = tmp;
					while(tmp) {
						if (tmp->insn.id == opt->id) {
							// delete this instruction
							if (tmp == prev) {
								// head of the list
								handle->mnem_list = tmp->next;
							} else {
								prev->next = tmp->next;
							}
							cs_mem_free(tmp);
							break;
						}
						prev = tmp;
						tmp = tmp->next;
					}
				}
			}
			return CS_ERR_OK;

		case CS_OPT_MODE:
			// verify if requested mode is valid
			if (value & cs_arch_disallowed_mode_mask[handle->arch]) {
				return CS_ERR_OPTION;
			}
			break;
	}

	return cs_arch_option[handle->arch](handle, type, value);
}

// generate @op_str for data instruction of SKIPDATA
#ifndef CAPSTONE_DIET
static void skipdata_opstr(char *opstr, const uint8_t *buffer, size_t size)
{
	char *p = opstr;
	int len;
	size_t i;
	size_t available = sizeof(((cs_insn*)NULL)->op_str);

	if (!size) {
		opstr[0] = '\0';
		return;
	}

	len = cs_snprintf(p, available, "0x%02x", buffer[0]);
	p+= len;
	available -= len;

	for(i = 1; i < size; i++) {
		len = cs_snprintf(p, available, ", 0x%02x", buffer[i]);
		if (len < 0) {
			break;
		}
		if ((size_t)len > available - 1) {
			break;
		}
		p+= len;
		available -= len;
	}
}
#endif

// dynamicly allocate memory to contain disasm insn
// NOTE: caller must free() the allocated memory itself to avoid memory leaking
CAPSTONE_EXPORT
size_t CAPSTONE_API cs_disasm(csh ud, const uint8_t *buffer, size_t size, uint64_t offset, size_t count, cs_insn **insn)
{
	struct cs_struct *handle;
	MCInst mci;
	uint16_t insn_size;
	size_t c = 0, i;
	unsigned int f = 0;	// index of the next instruction in the cache
	cs_insn *insn_cache;	// cache contains disassembled instructions
	void *total = NULL;
	size_t total_size = 0;	// total size of output buffer containing all insns
	bool r;
	void *tmp;
	size_t skipdata_bytes;
	uint64_t offset_org; // save all the original info of the buffer
	size_t size_org;
	const uint8_t *buffer_org;
	unsigned int cache_size = INSN_CACHE_SIZE;
	size_t next_offset;

	handle = (struct cs_struct *)(uintptr_t)ud;
	if (!handle) {
		// FIXME: how to handle this case:
		// handle->errnum = CS_ERR_HANDLE;
		return 0;
	}

	handle->errnum = CS_ERR_OK;

	// reset IT block of ARM structure
	if (handle->arch == CS_ARCH_ARM)
		handle->ITBlock.size = 0;

#ifdef CAPSTONE_USE_SYS_DYN_MEM
	if (count > 0 && count <= INSN_CACHE_SIZE)
		cache_size = (unsigned int) count;
#endif

	// save the original offset for SKIPDATA
	buffer_org = buffer;
	offset_org = offset;
	size_org = size;

	total_size = sizeof(cs_insn) * cache_size;
	total = cs_mem_malloc(total_size);
	if (total == NULL) {
		// insufficient memory
		handle->errnum = CS_ERR_MEM;
		return 0;
	}

	insn_cache = total;

	while (size > 0) {
		MCInst_Init(&mci);
		mci.csh = handle;

		// relative branches need to know the address & size of current insn
		mci.address = offset;

		if (handle->detail) {
			// allocate memory for @detail pointer
			insn_cache->detail = cs_mem_malloc(sizeof(cs_detail));
		} else {
			insn_cache->detail = NULL;
		}

		// save all the information for non-detailed mode
		mci.flat_insn = insn_cache;
		mci.flat_insn->address = offset;
#ifdef CAPSTONE_DIET
		// zero out mnemonic & op_str
		mci.flat_insn->mnemonic[0] = '\0';
		mci.flat_insn->op_str[0] = '\0';
#endif

		r = handle->disasm(ud, buffer, size, &mci, &insn_size, offset, handle->getinsn_info);
		if (r) {
			SStream ss;
			SStream_Init(&ss);

			mci.flat_insn->size = insn_size;

			// map internal instruction opcode to public insn ID

			handle->insn_id(handle, insn_cache, mci.Opcode);

			handle->printer(&mci, &ss, handle->printer_info);
			fill_insn(handle, insn_cache, ss.buffer, &mci, handle->post_printer, buffer);

			// adjust for pseudo opcode (X86)
			if (handle->arch == CS_ARCH_X86)
				insn_cache->id += mci.popcode_adjust;

			next_offset = insn_size;
		} else	{
			// encounter a broken instruction

			// free memory of @detail pointer
			if (handle->detail) {
				cs_mem_free(insn_cache->detail);
			}

			// if there is no request to skip data, or remaining data is too small,
			// then bail out
			if (!handle->skipdata || handle->skipdata_size > size)
				break;

			if (handle->skipdata_setup.callback) {
				skipdata_bytes = handle->skipdata_setup.callback(buffer_org, size_org,
						(size_t)(offset - offset_org), handle->skipdata_setup.user_data);
				if (skipdata_bytes > size)
					// remaining data is not enough
					break;

				if (!skipdata_bytes)
					// user requested not to skip data, so bail out
					break;
			} else
				skipdata_bytes = handle->skipdata_size;

			// we have to skip some amount of data, depending on arch & mode
			insn_cache->id = 0;	// invalid ID for this "data" instruction
			insn_cache->address = offset;
			insn_cache->size = (uint16_t)skipdata_bytes;
			memcpy(insn_cache->bytes, buffer, skipdata_bytes);
#ifdef CAPSTONE_DIET
			insn_cache->mnemonic[0] = '\0';
			insn_cache->op_str[0] = '\0';
#else
			strncpy(insn_cache->mnemonic, handle->skipdata_setup.mnemonic,
					sizeof(insn_cache->mnemonic) - 1);
			skipdata_opstr(insn_cache->op_str, buffer, skipdata_bytes);
#endif
			insn_cache->detail = NULL;

			next_offset = skipdata_bytes;
		}

		// one more instruction entering the cache
		f++;

		// one more instruction disassembled
		c++;
		if (count > 0 && c == count)
			// already got requested number of instructions
			break;

		if (f == cache_size) {
			// full cache, so expand the cache to contain incoming insns
			cache_size = cache_size * 8 / 5; // * 1.6 ~ golden ratio
			total_size += (sizeof(cs_insn) * cache_size);
			tmp = cs_mem_realloc(total, total_size);
			if (tmp == NULL) {	// insufficient memory
				if (handle->detail) {
					insn_cache = (cs_insn *)total;
					for (i = 0; i < c; i++, insn_cache++)
						cs_mem_free(insn_cache->detail);
				}

				cs_mem_free(total);
				*insn = NULL;
				handle->errnum = CS_ERR_MEM;
				return 0;
			}

			total = tmp;
			// continue to fill in the cache after the last instruction
			insn_cache = (cs_insn *)((char *)total + sizeof(cs_insn) * c);

			// reset f back to 0, so we fill in the cache from begining
			f = 0;
		} else
			insn_cache++;

		buffer += next_offset;
		size -= next_offset;
		offset += next_offset;
	}

	if (!c) {
		// we did not disassemble any instruction
		cs_mem_free(total);
		total = NULL;
	} else if (f != cache_size) {
		// total did not fully use the last cache, so downsize it
		tmp = cs_mem_realloc(total, total_size - (cache_size - f) * sizeof(*insn_cache));
		if (tmp == NULL) {	// insufficient memory
			// free all detail pointers
			if (handle->detail) {
				insn_cache = (cs_insn *)total;
				for (i = 0; i < c; i++, insn_cache++)
					cs_mem_free(insn_cache->detail);
			}

			cs_mem_free(total);
			*insn = NULL;

			handle->errnum = CS_ERR_MEM;
			return 0;
		}

		total = tmp;
	}

	*insn = total;

	return c;
}

CAPSTONE_EXPORT
CAPSTONE_DEPRECATED
size_t CAPSTONE_API cs_disasm_ex(csh ud, const uint8_t *buffer, size_t size, uint64_t offset, size_t count, cs_insn **insn)
{
	return cs_disasm(ud, buffer, size, offset, count, insn);
}

CAPSTONE_EXPORT
void CAPSTONE_API cs_free(cs_insn *insn, size_t count)
{
	size_t i;

	// free all detail pointers
	for (i = 0; i < count; i++)
		cs_mem_free(insn[i].detail);

	// then free pointer to cs_insn array
	cs_mem_free(insn);
}

CAPSTONE_EXPORT
cs_insn * CAPSTONE_API cs_malloc(csh ud)
{
	cs_insn *insn;
	struct cs_struct *handle = (struct cs_struct *)(uintptr_t)ud;

	insn = cs_mem_malloc(sizeof(cs_insn));
	if (!insn) {
		// insufficient memory
		handle->errnum = CS_ERR_MEM;
		return NULL;
	} else {
		if (handle->detail) {
			// allocate memory for @detail pointer
			insn->detail = cs_mem_malloc(sizeof(cs_detail));
			if (insn->detail == NULL) {	// insufficient memory
				cs_mem_free(insn);
				handle->errnum = CS_ERR_MEM;
				return NULL;
			}
		} else
			insn->detail = NULL;
	}

	return insn;
}

// iterator for instruction "single-stepping"
CAPSTONE_EXPORT
bool CAPSTONE_API cs_disasm_iter(csh ud, const uint8_t **code, size_t *size,
		uint64_t *address, cs_insn *insn)
{
	struct cs_struct *handle;
	uint16_t insn_size;
	MCInst mci;
	bool r;

	handle = (struct cs_struct *)(uintptr_t)ud;
	if (!handle) {
		return false;
	}

	handle->errnum = CS_ERR_OK;

	MCInst_Init(&mci);
	mci.csh = handle;

	// relative branches need to know the address & size of current insn
	mci.address = *address;

	// save all the information for non-detailed mode
	mci.flat_insn = insn;
	mci.flat_insn->address = *address;
#ifdef CAPSTONE_DIET
	// zero out mnemonic & op_str
	mci.flat_insn->mnemonic[0] = '\0';
	mci.flat_insn->op_str[0] = '\0';
#endif

	r = handle->disasm(ud, *code, *size, &mci, &insn_size, *address, handle->getinsn_info);
	if (r) {
		SStream ss;
		SStream_Init(&ss);

		mci.flat_insn->size = insn_size;

		// map internal instruction opcode to public insn ID
		handle->insn_id(handle, insn, mci.Opcode);

		handle->printer(&mci, &ss, handle->printer_info);

		fill_insn(handle, insn, ss.buffer, &mci, handle->post_printer, *code);

		// adjust for pseudo opcode (X86)
		if (handle->arch == CS_ARCH_X86)
			insn->id += mci.popcode_adjust;

		*code += insn_size;
		*size -= insn_size;
		*address += insn_size;
	} else { 	// encounter a broken instruction
		size_t skipdata_bytes;

		// if there is no request to skip data, or remaining data is too small,
		// then bail out
		if (!handle->skipdata || handle->skipdata_size > *size)
			return false;

		if (handle->skipdata_setup.callback) {
			skipdata_bytes = handle->skipdata_setup.callback(*code, *size,
					0, handle->skipdata_setup.user_data);
			if (skipdata_bytes > *size)
				// remaining data is not enough
				return false;

			if (!skipdata_bytes)
				// user requested not to skip data, so bail out
				return false;
		} else
			skipdata_bytes = handle->skipdata_size;

		// we have to skip some amount of data, depending on arch & mode
		insn->id = 0;	// invalid ID for this "data" instruction
		insn->address = *address;
		insn->size = (uint16_t)skipdata_bytes;
#ifdef CAPSTONE_DIET
		insn->mnemonic[0] = '\0';
		insn->op_str[0] = '\0';
#else
		memcpy(insn->bytes, *code, skipdata_bytes);
		strncpy(insn->mnemonic, handle->skipdata_setup.mnemonic,
				sizeof(insn->mnemonic) - 1);
		skipdata_opstr(insn->op_str, *code, skipdata_bytes);
#endif

		*code += skipdata_bytes;
		*size -= skipdata_bytes;
		*address += skipdata_bytes;
	}

	return true;
}

// return friendly name of regiser in a string
CAPSTONE_EXPORT
const char * CAPSTONE_API cs_reg_name(csh ud, unsigned int reg)
{
	struct cs_struct *handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle || handle->reg_name == NULL) {
		return NULL;
	}

	return handle->reg_name(ud, reg);
}

CAPSTONE_EXPORT
const char * CAPSTONE_API cs_insn_name(csh ud, unsigned int insn)
{
	struct cs_struct *handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle || handle->insn_name == NULL) {
		return NULL;
	}

	return handle->insn_name(ud, insn);
}

CAPSTONE_EXPORT
const char * CAPSTONE_API cs_group_name(csh ud, unsigned int group)
{
	struct cs_struct *handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle || handle->group_name == NULL) {
		return NULL;
	}

	return handle->group_name(ud, group);
}

CAPSTONE_EXPORT
bool CAPSTONE_API cs_insn_group(csh ud, const cs_insn *insn, unsigned int group_id)
{
	struct cs_struct *handle;
	if (!ud)
		return false;

	handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return false;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	return arr_exist8(insn->detail->groups, insn->detail->groups_count, group_id);
}

CAPSTONE_EXPORT
bool CAPSTONE_API cs_reg_read(csh ud, const cs_insn *insn, unsigned int reg_id)
{
	struct cs_struct *handle;
	if (!ud)
		return false;

	handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return false;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	return arr_exist(insn->detail->regs_read, insn->detail->regs_read_count, reg_id);
}

CAPSTONE_EXPORT
bool CAPSTONE_API cs_reg_write(csh ud, const cs_insn *insn, unsigned int reg_id)
{
	struct cs_struct *handle;
	if (!ud)
		return false;

	handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return false;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return false;
	}

	return arr_exist(insn->detail->regs_write, insn->detail->regs_write_count, reg_id);
}

CAPSTONE_EXPORT
int CAPSTONE_API cs_op_count(csh ud, const cs_insn *insn, unsigned int op_type)
{
	struct cs_struct *handle;
	unsigned int count = 0, i;
	if (!ud)
		return -1;

	handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return -1;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return -1;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return -1;
	}

	handle->errnum = CS_ERR_OK;

	switch (handle->arch) {
		default:
			handle->errnum = CS_ERR_HANDLE;
			return -1;
		case CS_ARCH_ARM:
			for (i = 0; i < insn->detail->arm.op_count; i++)
				if (insn->detail->arm.operands[i].type == (arm_op_type)op_type)
					count++;
			break;
		case CS_ARCH_ARM64:
			for (i = 0; i < insn->detail->arm64.op_count; i++)
				if (insn->detail->arm64.operands[i].type == (arm64_op_type)op_type)
					count++;
			break;
		case CS_ARCH_X86:
			for (i = 0; i < insn->detail->x86.op_count; i++)
				if (insn->detail->x86.operands[i].type == (x86_op_type)op_type)
					count++;
			break;
		case CS_ARCH_MIPS:
			for (i = 0; i < insn->detail->mips.op_count; i++)
				if (insn->detail->mips.operands[i].type == (mips_op_type)op_type)
					count++;
			break;
		case CS_ARCH_PPC:
			for (i = 0; i < insn->detail->ppc.op_count; i++)
				if (insn->detail->ppc.operands[i].type == (ppc_op_type)op_type)
					count++;
			break;
		case CS_ARCH_SPARC:
			for (i = 0; i < insn->detail->sparc.op_count; i++)
				if (insn->detail->sparc.operands[i].type == (sparc_op_type)op_type)
					count++;
			break;
		case CS_ARCH_SYSZ:
			for (i = 0; i < insn->detail->sysz.op_count; i++)
				if (insn->detail->sysz.operands[i].type == (sysz_op_type)op_type)
					count++;
			break;
		case CS_ARCH_XCORE:
			for (i = 0; i < insn->detail->xcore.op_count; i++)
				if (insn->detail->xcore.operands[i].type == (xcore_op_type)op_type)
					count++;
			break;
		case CS_ARCH_M68K:
			for (i = 0; i < insn->detail->m68k.op_count; i++)
				if (insn->detail->m68k.operands[i].type == (m68k_op_type)op_type)
					count++;
			break;
		case CS_ARCH_TMS320C64X:
			for (i = 0; i < insn->detail->tms320c64x.op_count; i++)
				if (insn->detail->tms320c64x.operands[i].type == (tms320c64x_op_type)op_type)
					count++;
			break;
		case CS_ARCH_M680X:
			for (i = 0; i < insn->detail->m680x.op_count; i++)
				if (insn->detail->m680x.operands[i].type == (m680x_op_type)op_type)
					count++;
			break;
		case CS_ARCH_EVM:
#if 0
			for (i = 0; i < insn->detail->evm.op_count; i++)
				if (insn->detail->evm.operands[i].type == (evm_op_type)op_type)
					count++;
#endif
			break;
	}

	return count;
}

CAPSTONE_EXPORT
int CAPSTONE_API cs_op_index(csh ud, const cs_insn *insn, unsigned int op_type,
		unsigned int post)
{
	struct cs_struct *handle;
	unsigned int count = 0, i;
	if (!ud)
		return -1;

	handle = (struct cs_struct *)(uintptr_t)ud;

	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return -1;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return -1;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return -1;
	}

	handle->errnum = CS_ERR_OK;

	switch (handle->arch) {
		default:
			handle->errnum = CS_ERR_HANDLE;
			return -1;
		case CS_ARCH_ARM:
			for (i = 0; i < insn->detail->arm.op_count; i++) {
				if (insn->detail->arm.operands[i].type == (arm_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_ARM64:
			for (i = 0; i < insn->detail->arm64.op_count; i++) {
				if (insn->detail->arm64.operands[i].type == (arm64_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_X86:
			for (i = 0; i < insn->detail->x86.op_count; i++) {
				if (insn->detail->x86.operands[i].type == (x86_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_MIPS:
			for (i = 0; i < insn->detail->mips.op_count; i++) {
				if (insn->detail->mips.operands[i].type == (mips_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_PPC:
			for (i = 0; i < insn->detail->ppc.op_count; i++) {
				if (insn->detail->ppc.operands[i].type == (ppc_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_SPARC:
			for (i = 0; i < insn->detail->sparc.op_count; i++) {
				if (insn->detail->sparc.operands[i].type == (sparc_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_SYSZ:
			for (i = 0; i < insn->detail->sysz.op_count; i++) {
				if (insn->detail->sysz.operands[i].type == (sysz_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_XCORE:
			for (i = 0; i < insn->detail->xcore.op_count; i++) {
				if (insn->detail->xcore.operands[i].type == (xcore_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_M68K:
			for (i = 0; i < insn->detail->m68k.op_count; i++) {
				if (insn->detail->m68k.operands[i].type == (m68k_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_TMS320C64X:
			for (i = 0; i < insn->detail->tms320c64x.op_count; i++) {
				if (insn->detail->tms320c64x.operands[i].type == (tms320c64x_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_M680X:
			for (i = 0; i < insn->detail->m680x.op_count; i++) {
				if (insn->detail->m680x.operands[i].type == (m680x_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
			break;
		case CS_ARCH_EVM:
#if 0
			for (i = 0; i < insn->detail->evm.op_count; i++) {
				if (insn->detail->evm.operands[i].type == (evm_op_type)op_type)
					count++;
				if (count == post)
					return i;
			}
#endif
			break;
	}

	return -1;
}

CAPSTONE_EXPORT
cs_err CAPSTONE_API cs_regs_access(csh ud, const cs_insn *insn,
		cs_regs regs_read, uint8_t *regs_read_count,
		cs_regs regs_write, uint8_t *regs_write_count)
{
	struct cs_struct *handle;

	if (!ud)
		return -1;

	handle = (struct cs_struct *)(uintptr_t)ud;

#ifdef CAPSTONE_DIET
	// This API does not work in DIET mode
	handle->errnum = CS_ERR_DIET;
	return CS_ERR_DIET;
#else
	if (!handle->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return CS_ERR_DETAIL;
	}

	if (!insn->id) {
		handle->errnum = CS_ERR_SKIPDATA;
		return CS_ERR_SKIPDATA;
	}

	if (!insn->detail) {
		handle->errnum = CS_ERR_DETAIL;
		return CS_ERR_DETAIL;
	}

	if (handle->reg_access) {
		handle->reg_access(insn, regs_read, regs_read_count, regs_write, regs_write_count);
	} else {
		// this arch is unsupported yet
		handle->errnum = CS_ERR_ARCH;
		return CS_ERR_ARCH;
	}

	return CS_ERR_OK;
#endif
}
