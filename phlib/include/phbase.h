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
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winsta.lib")
#endif

#if !defined(_PHLIB_)
#define PHLIBAPI __declspec(dllimport)
#else
#define PHLIBAPI
#endif

#ifdef __clang__
#define PH_CLANG_STRINGIFYX(x)         #x
#define PH_CLANG_STRINGIFY(x)          PH_CLANG_STRINGIFYX(x)
#define PH_CLANG_DIAGNOSTIC_PUSH()     _Pragma("clang diagnostic push")
#define PH_CLANG_DIAGNOSTIC_IGNORED(x) _Pragma(PH_CLANG_STRINGIFY(clang diagnostic ignored x))
#define PH_CLANG_DIAGNOSTIC_POP()      _Pragma("clang diagnostic pop")
#else
#define PH_CLANG_DIAGNOSTIC_PUSH()
#define PH_CLANG_DIAGNOSTIC_IGNORED(x)
#define PH_CLANG_DIAGNOSTIC_POP()
#endif

#include <phnt_windows.h>
#include <phnt.h>
#include <phsup.h>
#include <ref.h>
#include <queuedlock.h>
#include <phconfig.h>
#include <phbasesup.h>
#include <phdata.h>

#endif
