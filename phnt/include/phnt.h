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
#define PHNT_WINDOWS_ANCIENT 0
#define PHNT_WINDOWS_XP 51 // August, 2001
#define PHNT_WINDOWS_SERVER_2003 52 // April, 2003
#define PHNT_WINDOWS_VISTA 60 // November, 2006
#define PHNT_WINDOWS_7 61 // July, 2009
#define PHNT_WINDOWS_8 62 // August, 2012
#define PHNT_WINDOWS_8_1 63 // August, 2013
#define PHNT_WINDOWS_10 100 // July, 2015            // Version 1507, Build 10240
#define PHNT_WINDOWS_10_TH2 101 // November, 2015    // Version 1511, Build 10586
#define PHNT_WINDOWS_10_RS1 102 // August, 2016      // Version 1607, Build 14393
#define PHNT_WINDOWS_10_RS2 103 // April, 2017       // Version 1703, Build 15063
#define PHNT_WINDOWS_10_RS3 104 // October, 2017     // Version 1709, Build 16299
#define PHNT_WINDOWS_10_RS4 105 // April, 2018       // Version 1803, Build 17134
#define PHNT_WINDOWS_10_RS5 106 // November, 2018    // Version 1809, Build 17763
#define PHNT_WINDOWS_10_19H1 107 // May, 2019        // Version 1903, Build 18362
#define PHNT_WINDOWS_10_19H2 108 // November, 2019   // Version 1909, Build 18363
#define PHNT_WINDOWS_10_20H1 109 // May, 2020        // Version 2004, Build 19041
#define PHNT_WINDOWS_10_20H2 110 // October, 2020    // Build 19042
#define PHNT_WINDOWS_10_21H1 111 // May, 2021        // Build 19043
#define PHNT_WINDOWS_10_21H2 112 // November, 2021   // Build 19044
#define PHNT_WINDOWS_10_22H2 113 // October, 2022    // Build 19045
#define PHNT_WINDOWS_11 114 // October, 2021         // Build 22000
#define PHNT_WINDOWS_11_22H2 115 // September, 2022  // Build 22621
#define PHNT_WINDOWS_11_23H2 116 // October, 2023    // Build 22631
#define PHNT_WINDOWS_11_24H2 117 // October, 2024    // Build 26100
#define PHNT_WINDOWS_NEW ULONG_MAX

#ifndef PHNT_MODE
#define PHNT_MODE PHNT_MODE_USER
#endif

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WINDOWS_NEW
#endif

//
// Options
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)
//#ifndef PHNT_NO_INLINE_INIT_STRING
//#define PHNT_NO_INLINE_INIT_STRING
//#endif // !PHNT_NO_INLINE_INIT_STRING
#ifndef PHNT_INLINE_TYPEDEFS
#define PHNT_INLINE_TYPEDEFS
#endif // !PHNT_INLINE_TYPEDEFS
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

EXTERN_C_START

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <phnt_ntdef.h>
#include <ntnls.h>
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#include <ntkeapi.h>
#include <ntldr.h>
#include <ntexapi.h>

#include <ntbcd.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <ntdbg.h>
#include <ntimage.h>
#include <ntioapi.h>
#include <ntlsa.h>
#include <ntlpcapi.h>
#include <ntmisc.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#include <ntsam.h>
#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntuser.h>
#include <ntwmi.h>
#include <ntwow64.h>
#include <ntxcapi.h>
#include <ntzwapi.h>
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

EXTERN_C_END

static_assert(__alignof(LARGE_INTEGER) == 8, "Windows headers require the default packing option. Changing the packing can lead to memory corruption.");
static_assert(__alignof(PROCESS_CYCLE_TIME_INFORMATION) == 8, "PHNT headers require the default packing option. Changing the packing can lead to memory corruption.");

#endif
