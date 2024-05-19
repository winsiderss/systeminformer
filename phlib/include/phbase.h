/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2016-2023
 *
 */

#ifndef _PH_PHBASE_H
#define _PH_PHBASE_H

#ifndef PHLIB_NO_DEFAULT_LIB
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "comctl32.lib")
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#if !defined(_PHLIB_)
#define PHLIBAPI __declspec(dllimport)
#else
#define PHLIBAPI
#endif

#include <phnt_windows.h>
#include <phnt.h>
#include <phsup.h>
#include <ref.h>
#include <queuedlock.h>

#include <stdlib.h>

#include <phconfig.h>
#include <phbasesup.h>
#include <phdata.h>

#endif
