#ifndef NTIMPORT_H
#define NTIMPORT_H

#define PHNT_WIN2K 50
#define PHNT_WINXP 51
#define PHNT_WS03 52
#define PHNT_VISTA 60
#define PHNT_WIN7 61

#define PHNT_VERSION PHNT_WIN2K

// Note: definitions marked with an asterisk have been 
// reverse-engineered.

#include <ntbasic.h>
#include <ntcm.h>
#include <ntdbg.h>
#include <ntexapi.h>
#include <ntioapi.h>
#include <ntkeapi.h>
#include <ntktmapi.h>
#include <ntlpcapi.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntpsapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <ntxcapi.h>

#include <ntlsa.h>
#include <ntmisc.h>

#include <rev.h>

#ifdef NTIMPORT_PRIVATE
#define EXT
#define EQNULL = NULL
#else
#define EXT extern
#define EQNULL
#endif

// Only functions appearing in Windows XP and below may be 
// imported normally. The other functions are imported here.

EXT _NtGetNextProcess NtGetNextProcess EQNULL; // WS03
EXT _NtGetNextThread NtGetNextThread EQNULL; // WS03
EXT _NtQueryInformationEnlistment NtQueryInformationEnlistment EQNULL; // VISTA
EXT _NtQueryInformationResourceManager NtQueryInformationResourceManager EQNULL; // VISTA
EXT _NtQueryInformationTransaction NtQueryInformationTransaction EQNULL; // VISTA
EXT _NtQueryInformationTransactionManager NtQueryInformationTransactionManager EQNULL; // VISTA

BOOLEAN PhInitializeImports();

#endif
