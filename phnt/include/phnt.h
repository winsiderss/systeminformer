/*
 * NT Header annotations
 *
 * This file is part of System Informer.
 */

#ifndef _PHNT_H
#define _PHNT_H

// This header file provides access to NT APIs.

// Definitions are annotated to indicate their source. If a definition is not annotated, it has been
// retrieved from an official Microsoft source (NT headers, DDK headers, winnt.h).

// * "winbase" indicates that a definition has been reconstructed from a Win32-ized NT definition in
//   winbase.h.
// * "rev" indicates that a definition has been reverse-engineered.
// * "dbg" indicates that a definition has been obtained from a debug message or assertion in a
//   checked build of the kernel or file.

// Reliability:
// 1. No annotation.
// 2. dbg.
// 3. symbols, private. Types may be incorrect.
// 4. winbase. Names and types may be incorrect.
// 5. rev.

// Mode
#define PHNT_MODE_KERNEL 0
#define PHNT_MODE_USER 1

// Version
#define PHNT_WIN2K 50
#define PHNT_WINXP 51
#define PHNT_WS03 52
#define PHNT_VISTA 60
#define PHNT_WIN7 61
#define PHNT_WIN8 62
#define PHNT_WINBLUE 63
#define PHNT_THRESHOLD 100
#define PHNT_THRESHOLD2 101
#define PHNT_REDSTONE 102
#define PHNT_REDSTONE2 103
#define PHNT_REDSTONE3 104
#define PHNT_REDSTONE4 105
#define PHNT_REDSTONE5 106
#define PHNT_19H1 107
#define PHNT_19H2 108
#define PHNT_20H1 109
#define PHNT_20H2 110
#define PHNT_21H1 111
#define PHNT_WIN10_21H2 112
#define PHNT_WIN10_22H2 113
#define PHNT_WIN11 114
#define PHNT_WIN11_22H2 115
#define PHNT_WIN11_23H2 116
#define PHNT_WIN11_24H2 117

#ifndef PHNT_MODE
#define PHNT_MODE PHNT_MODE_USER
#endif

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WIN11_24H2
#endif

// Options

#if (PHNT_MODE != PHNT_MODE_KERNEL)
//#define PHNT_NO_INLINE_INIT_STRING
#ifndef PHNT_INLINE_TYPEDEFS
#define PHNT_INLINE_TYPEDEFS
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <phnt_ntdef.h>
#include <ntnls.h>
#endif

#include <ntkeapi.h>
#include <ntldr.h>
#include <ntexapi.h>

#include <ntbcd.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <ntdbg.h>
#include <ntioapi.h>
#include <ntlpcapi.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#include <ntimage.h>
#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntxcapi.h>
#include <ntwow64.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntmisc.h>
#include <ntwmi.h>
#include <ntzwapi.h>
#endif

#ifdef __cplusplus
}
#endif

static_assert(__alignof(LARGE_INTEGER) == 8, "Windows headers require the default packing option. Changing the packing can lead to memory corruption.");
static_assert(__alignof(PROCESS_CYCLE_TIME_INFORMATION) == 8, "PHNT headers require the default packing option. Changing the packing can lead to memory corruption.");

#endif
