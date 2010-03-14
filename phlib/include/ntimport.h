#ifndef NTIMPORT_H
#define NTIMPORT_H

#include <ntbasic.h>
#include <ntexapi.h>
#include <ntioapi.h>
#include <ntkeapi.h>
#include <ntktmapi.h>
#include <ntlpcapi.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>
#include <ntseapi.h>

#include <ntdbg.h>
#include <ntrtl.h>
#include <ntmisc.h>

#include <ntlsa.h>

#ifdef NTIMPORT_PRIVATE
#define EXT
#define EQNULL = NULL
#else
#define EXT extern
#define EQNULL
#endif

// All functions appearing only in Windows XP and later must be 
// placed on this list. Otherwise, they may be imported normally.

EXT _NtCreateDebugObject NtCreateDebugObject EQNULL;
EXT _NtCreateKeyedEvent NtCreateKeyedEvent EQNULL;
EXT _NtDebugActiveProcess NtDebugActiveProcess EQNULL;
EXT _NtGetNextProcess NtGetNextProcess EQNULL;
EXT _NtGetNextThread NtGetNextThread EQNULL;
EXT _NtIsProcessInJob NtIsProcessInJob EQNULL;
EXT _NtOpenKeyedEvent NtOpenKeyedEvent EQNULL;
EXT _NtQueryInformationEnlistment NtQueryInformationEnlistment EQNULL;
EXT _NtQueryInformationResourceManager NtQueryInformationResourceManager EQNULL;
EXT _NtQueryInformationTransaction NtQueryInformationTransaction EQNULL;
EXT _NtQueryInformationTransactionManager NtQueryInformationTransactionManager EQNULL;
EXT _NtReleaseKeyedEvent NtReleaseKeyedEvent EQNULL;
EXT _NtRemoveProcessDebug NtRemoveProcessDebug EQNULL;
EXT _NtResumeProcess NtResumeProcess EQNULL;
EXT _NtSetInformationDebugObject NtSetInformationDebugObject EQNULL;
EXT _NtSuspendProcess NtSuspendProcess EQNULL;
EXT _NtWaitForKeyedEvent NtWaitForKeyedEvent EQNULL;

BOOLEAN PhInitializeImports();

#endif
