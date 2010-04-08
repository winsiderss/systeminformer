/*
 * Process Hacker Driver - 
 *   hooks
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

#ifndef _HOOK_H
#define _HOOK_H

#include "kph.h"
#include "ob.h"

#define KPH_DEFINE_HOOK_CALL(Name, Arguments, Hook) \
    __declspec(naked) Name(Arguments) \
    { \
        __asm lea   eax, Hook \
        __asm mov   eax, [eax+KPH_HOOK.Function] \
        __asm add   eax, 5 \
        __asm push  ebp \
        __asm mov   ebp, esp \
        __asm jmp   eax \
    } \

typedef struct _KPH_HOOK
{
    /* The address of the hooked function.
       Should NOT be a function that is callable above PASSIVE_LEVEL. */
    PVOID Function;
    /* The address of the new function. */
    PVOID Target;
    /* Whether the function is hooked. */
    BOOLEAN Hooked;
    /* The original first 10 bytes. */
    CHAR Bytes[10];
} KPH_HOOK, *PKPH_HOOK;

typedef struct _KPH_OB_OPEN_HOOK
{
    /* The object type that is being hooked. */
    POBJECT_TYPE ObjectType;
    /* The original open procedure. */
    PVOID Function;
    /* The new open procedure for NT 5.1 (XP). */
    OB_OPEN_METHOD_51 Target51;
    /* The new open procedure for NT 6.1 and above (Vista, 7 or higher). */
    OB_OPEN_METHOD_60 Target60;
    /* Whether the open procedure is hooked. */
    BOOLEAN Hooked;
} KPH_OB_OPEN_HOOK, *PKPH_OB_OPEN_HOOK;

NTSTATUS KphHookInit();

VOID KphInitializeHook(
    __out PKPH_HOOK Hook,
    __in PVOID Function,
    __in PVOID Target
    );

NTSTATUS KphHook(
    __inout PKPH_HOOK Hook
    );

NTSTATUS KphUnhook(
    __inout PKPH_HOOK Hook
    );

NTSTATUS NTAPI KphObOpenCall(
    __in PKPH_OB_OPEN_HOOK ObOpenHook,
    __in OB_OPEN_REASON OpenReason,
    __in KPROCESSOR_MODE AccessMode,
    __in PEPROCESS Process,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess,
    __in ULONG HandleCount
    );

VOID KphInitializeObOpenHook(
    __inout PKPH_OB_OPEN_HOOK ObOpenHook,
    __in POBJECT_TYPE ObjectType,
    __in PVOID Target51,
    __in PVOID Target60
    );

NTSTATUS KphObOpenHook(
    __inout PKPH_OB_OPEN_HOOK ObOpenHook
    );

NTSTATUS KphObOpenUnhook(
    __inout PKPH_OB_OPEN_HOOK ObOpenHook
    );

#endif
