/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2023
 *
 */

#ifndef _PH_WSLSUP_H
#define _PH_WSLSUP_H

EXTERN_C_START

BOOLEAN PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName
    );

_Success_(return)
BOOLEAN PhCreateProcessLxss(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssCommandLine,
    _Out_ PPH_STRING *Result
    );

EXTERN_C_END

#endif
