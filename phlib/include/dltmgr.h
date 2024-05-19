/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#ifndef _PH_DLTMGR_H
#define _PH_DLTMGR_H

typedef struct _PH_SINGLE_DELTA
{
    FLOAT Value;
    FLOAT Delta;
} PH_SINGLE_DELTA, *PPH_SINGLE_DELTA;

typedef struct _PH_UINT32_DELTA
{
    ULONG Value;
    ULONG Delta;
} PH_UINT32_DELTA, *PPH_UINT32_DELTA;

typedef struct _PH_UINT64_DELTA
{
    ULONG64 Value;
    ULONG64 Delta;
} PH_UINT64_DELTA, *PPH_UINT64_DELTA;

typedef struct _PH_UINTPTR_DELTA
{
    ULONG_PTR Value;
    ULONG_PTR Delta;
} PH_UINTPTR_DELTA, *PPH_UINTPTR_DELTA;

#define PhInitializeDelta(DltMgr) \
    ((DltMgr)->Value = 0, (DltMgr)->Delta = 0)

#define PhUpdateDelta(DltMgr, NewValue) \
    ((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)

#define PH_SINGLE_DELTA_INIT { 0.0F, 0.0F }
#define PH_UINT32_DELTA_INIT { 0UL, 0UL }
#define PH_UINT64_DELTA_INIT { 0ULL, 0ULL }
#define PH_UINTPTR_DELTA_INIT { 0, 0 }

#endif
