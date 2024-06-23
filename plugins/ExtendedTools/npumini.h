/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2017-2022
 *     jxy-s   2024
 *
 */

#ifndef NPUMINI_H
#define NPUMINI_H

BOOLEAN EtpNpuListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl EtpNpuListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl EtpNpuListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

#endif
