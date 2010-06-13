#ifndef NTIMPORT_H
#define NTIMPORT_H

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
