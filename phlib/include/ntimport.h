#ifndef _NTIMPORT_H
#define _NTIMPORT_H

#ifdef NTIMPORT_PRIVATE
#define EXT
#define EQNULL = NULL
#else
#define EXT extern
#define EQNULL
#endif

// Only functions appearing in Windows XP and below may be 
// imported normally. The other functions are imported here.

#if !(PHNT_VERSION >= PHNT_WS03)
EXT _NtGetNextProcess NtGetNextProcess EQNULL;
EXT _NtGetNextThread NtGetNextThread EQNULL;
#endif

#if !(PHNT_VERSION >= PHNT_VISTA)
EXT _NtQueryInformationEnlistment NtQueryInformationEnlistment EQNULL;
EXT _NtQueryInformationResourceManager NtQueryInformationResourceManager EQNULL;
EXT _NtQueryInformationTransaction NtQueryInformationTransaction EQNULL;
EXT _NtQueryInformationTransactionManager NtQueryInformationTransactionManager EQNULL;
#endif

BOOLEAN PhInitializeImports();

#endif
