/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#pragma once
#include <kph.h>
#include <kphmsg.h>

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupInformer(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeInformer(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerAllowed(
    _In_ ULONG Index,
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    );

#define KphInformerEnabled(name, proc)                                         \
    KphInformerAllowed(KPH_INFORMER_INDEX(name), proc, NULL)

#define KphInformerEnabled2(name, actor, target)                               \
    KphInformerAllowed(KPH_INFORMER_INDEX(name), actor, target)

_IRQL_requires_max_(DISPATCH_LEVEL)
KPH_INFORMER_OPTIONS KphInformerOptions(
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    );

#define KphInformerOpts(proc)                                                  \
    KphInformerOptions(proc, NULL)

#define KphInformerOpts2(proc, target)                                         \
    KphInformerOptions(proc, target)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerSettings(
    _In_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerProcessSettings(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerProcessSettings(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    );

// mini-filter informer

extern PFLT_FILTER KphFltFilter;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltRegister(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFltUnregister(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltInformerStart(
    VOID
    );

// process informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphProcessInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphProcessInformerStop(
    VOID
    );

// thread informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphThreadInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphThreadInformerStop(
    VOID
    );

// image informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphImageInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphImageInformerStop(
    VOID
    );

// object informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphObjectInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphObjectInformerStop(
    VOID
    );

// registry informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphRegistryInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphRegistryInformerStop(
    VOID
    );

// debug informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDebugInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDebugInformerStop(
    VOID
    );
