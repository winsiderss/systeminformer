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

// Version

#define PHNT_WIN2K 50
#define PHNT_WINXP 51
#define PHNT_WS03 52
#define PHNT_VISTA 60
#define PHNT_WIN7 61

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WINXP
#endif

// Options

//#define PHNT_NO_INLINE_INIT_STRING

#ifdef __cplusplus
extern "C" {
#endif

#include <ntbasic.h>
#include <ntnls.h>

#include <ntcm.h>
#include <ntdbg.h>
#include <ntexapi.h>
#include <ntioapi.h>
#include <ntkeapi.h>
#include <ntlpcapi.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntpsapi.h>
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

#include <rev.h>

#ifdef __cplusplus
}
#endif

#endif
