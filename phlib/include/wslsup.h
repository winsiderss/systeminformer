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

NTSTATUS PhWslQueryDistroProcessCommandLine(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    );

NTSTATUS PhWslQueryDistroProcessEnvironment(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    );

NTSTATUS PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName
    );

NTSTATUS PhCreateProcessLxss(
    _In_ PPH_STRING DistributionGuid,
    _In_ PPH_STRING DistributionName,
    _In_ PPH_STRING DistributionCommand,
    _Out_ PPH_STRING* Result
    );

EXTERN_C_END

#endif
