/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *     dmex    2022-2023
 *
 */

#ifndef PH_KSISUP_H
#define PH_KSISUP_H

VOID PhShowKsiStatus(
    VOID
    );

VOID PhInitializeKsi(
    VOID
    );

VOID PhDestroyKsi(
    VOID
    );

VOID PhShowKsiUnsupportedError(
    _In_ HWND ParentWindowHandle
    );

#endif
