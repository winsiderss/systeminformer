/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once
#include <wdm.h>

typedef enum _KPH_OSVERSION
{
    KphWinUnknown = 0,
    KphWin7,
    KphWin8,
    KphWin81,
    KphWin10

} KPH_OSVERSION;

extern RTL_OSVERSIONINFOEXW KphOsVersionInfo;
extern USHORT KphOsRevision;
extern KPH_OSVERSION KphOsVersion;
