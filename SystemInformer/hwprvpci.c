/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <hwprvp.h>

/**
 * Writes the PCI error policy of the bridges the pciconfig script lists, for the hardware report.
 *
 * \param Builder The report.
 */
VOID PhHwWriteBridgeDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendStringBuilder2(Builder, L"Not collected\n");
}
