#ifndef _PH_PHNT_H
#define _PH_PHNT_H

// This header file provides access to NT APIs.

// Definitions are annotated to indicate their source.
// If a definition is not annotated, it has been retrieved 
// from an official Microsoft source (NT headers, DDK headers, winnt.h).

// "winbase" indicates that a definition has been reconstructed from 
// a Win32-ized NT definition in winbase.h.
// "rev" indicates that a definition has been reverse-engineered.
// "dbg" indicates that a definition has been obtained from a debug 
// message or assertion in a checked build of the kernel or file.

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

#ifndef PHNT_MODE
#define PHNT_MODE PHNT_MODE_USER
#endif

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WINXP
#endif

// Options

//#define PHNT_NO_INLINE_INIT_STRING

#ifdef __cplusplus
extern "C" {
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#include <ntbasic.h>
#include <ntnls.h>
#include <ntexapi.h>
#include <ntkeapi.h>
#include <ntmmapi.h>

#endif

#include <ntobapi.h>
#include <ntpsapi.h>

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#include <ntcm.h>
#include <ntdbg.h>
#include <ntioapi.h>
#include <ntlpcapi.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntxcapi.h>

#include <ntwow64.h>

#include <ntlsa.h>
#include <ntsam.h>

#include <ntmisc.h>

#include <ntzwapi.h>

#include <rev.h>

#endif

#ifdef __cplusplus
}
#endif

#endif
