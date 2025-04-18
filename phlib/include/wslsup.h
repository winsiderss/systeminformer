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

_Success_(return)
BOOLEAN PhWslQueryDistroProcessCommandLine(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    );

_Success_(return)
BOOLEAN PhWslQueryDistroProcessEnvironment(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    );

BOOLEAN PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName
    );

BOOLEAN PhCreateProcessLxss(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssCommandLine,
    _Out_ PPH_STRING *Result
    );

EXTERN_C_END

#endif
