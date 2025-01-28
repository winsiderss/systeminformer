/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021
 *
 */

#pragma once

EXTERN_C_START

EXTERN_C
BOOLEAN
NTAPI
PvGetTlshBufferHash(
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ char** HashResult
    );

EXTERN_C
BOOLEAN
NTAPI
PvGetTlshFileHash(
    _In_ HANDLE FileHandle,
    _Out_ char** HashResult
    );

EXTERN_C_END
