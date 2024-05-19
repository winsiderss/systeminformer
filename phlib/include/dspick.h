/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#ifndef _PH_DSPICK_H
#define _PH_DSPICK_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_DSPICK_MULTISELECT 0x1

typedef struct _PH_DSPICK_OBJECT
{
    PPH_STRING Name;
    PSID Sid;
} PH_DSPICK_OBJECT, *PPH_DSPICK_OBJECT;

typedef struct _PH_DSPICK_OBJECTS
{
    ULONG NumberOfObjects;
    PH_DSPICK_OBJECT Objects[1];
} PH_DSPICK_OBJECTS, *PPH_DSPICK_OBJECTS;

PHLIBAPI
VOID PhFreeDsObjectPickerDialog(
    _In_ PVOID PickerDialog
    );

PHLIBAPI
PVOID PhCreateDsObjectPickerDialog(
    _In_ ULONG Flags
    );

_Success_(return)
PHLIBAPI
BOOLEAN PhShowDsObjectPickerDialog(
    _In_ HWND hWnd,
    _In_ PVOID PickerDialog,
    _Out_ PPH_DSPICK_OBJECTS *Objects
    );

PHLIBAPI
VOID PhFreeDsObjectPickerObjects(
    _In_ PPH_DSPICK_OBJECTS Objects
    );

#ifdef __cplusplus
}
#endif

#endif
