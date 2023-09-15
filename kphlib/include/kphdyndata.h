/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#pragma once

#include <kphdyn.h>

_Must_inspect_result_
NTSTATUS KphDynDataGetConfiguration(
    _In_ PKPH_DYNDATA DynData,
    _In_ USHORT MajorVersion,
    _In_ USHORT MinorVersion,
    _In_ USHORT BuildNumber,
    _In_ USHORT Revision,
    _Out_opt_ PKPH_DYN_CONFIGURATION* Config
    );
