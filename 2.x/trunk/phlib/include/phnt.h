#ifndef _PH_PHNT_H
#define _PH_PHNT_H

// This header file provides access to NT APIs.

#define PHNT_WIN2K 50
#define PHNT_WINXP 51
#define PHNT_WS03 52
#define PHNT_VISTA 60
#define PHNT_WIN7 61

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WINXP
#endif

#include <ntbasic.h>
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

#include <ntlsa.h>
#include <ntmisc.h>
#include <winsta.h>

#include <rev.h>

#endif
