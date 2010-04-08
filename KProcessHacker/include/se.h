/*
 * Process Hacker Driver - 
 *   memory manager
 * 
 * Copyright (C) 2009 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SE_H
#define _SE_H

#include "types.h"

extern POBJECT_TYPE *SeTokenObjectType;

/* Was 0x38 on Vista, appears to be 0xc8 on 7. */
#define AUX_ACCESS_DATA_SIZE (0xc8)

typedef PVOID PAUX_ACCESS_DATA;

/* FUNCTION DEFS */

NTKERNELAPI NTSTATUS NTAPI SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_ACCESS_DATA AuxData,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING Mapping
    );

NTKERNELAPI VOID NTAPI SeDeleteAccessState(
    PACCESS_STATE AccessState
    );

/* STRUCTS */

typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
{
    POBJECT_NAME_INFORMATION ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;

#endif
