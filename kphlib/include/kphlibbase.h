/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2026
 *
 */

#pragma once

#include <sistatus.h>

#ifdef _KERNEL_MODE
#pragma warning(push)
#pragma warning(disable: 5103) // invalid preprocessing token (/Zc:preprocessor)
#include <ntifs.h>
#include <ntintsafe.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#pragma warning(pop)

//
// Redefined with integer arithmetic to avoid undefined behavior on pointer
// overflow. Clang elides comparisons that depend on it, MSVC does not. The
// result is otherwise identical to the WDK definition.
//
#undef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((ULONG_PTR)(P) + (ULONG_PTR)(I)))
#else
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#pragma warning(disable : 4324)
#pragma warning(disable : 4115)
#include <phnt_windows.h>
#include <phnt.h>
#include <ntintsafe.h>
#pragma warning(pop)
#endif
