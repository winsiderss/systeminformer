/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020
 *
 */

#include <kph.h>
#include <dyndata.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphGetSystemRoutineAddress)
#pragma alloc_text(PAGE, KphDynamicImport)
#endif

/**
 * Dynamically imports routines.
 */
VOID KphDynamicImport(
    VOID
    )
{
    PAGED_CODE();

    KphDynPsGetProcessProtection = (PPS_GET_PROCESS_PROTECTION)KphGetSystemRoutineAddress(L"PsGetProcessProtection");
    KphDynRtlImageNtHeaderEx = (PRTL_IMAGE_NT_HEADER_EX)KphGetSystemRoutineAddress(L"RtlImageNtHeaderEx");
}

/**
 * Retrieves the address of a function exported by NTOS or HAL.
 *
 * \param SystemRoutineName The name of the function.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
PVOID KphGetSystemRoutineAddress(
    _In_ PWSTR SystemRoutineName
    )
{
    UNICODE_STRING systemRoutineName;

    PAGED_CODE();

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
}
