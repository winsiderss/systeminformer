/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020
 *
 */

#ifndef _MAIN_H
#define _MAIN_H

#include <ph.h>
#include <svcsup.h>

extern PVOID __ImageBase;
extern PPH_STRING CommandType;
extern PPH_STRING CommandObject;
extern PPH_STRING CommandAction;
extern PPH_STRING CommandValue;

NTSTATUS PhCommandModeStart(
    VOID
    );

#endif
