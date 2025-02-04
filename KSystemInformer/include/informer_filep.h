/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
 *
 */
#pragma once

// informer_fileop.c

_Function_class_(PFLT_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
FLT_POSTOP_CALLBACK_STATUS FLTAPI KphpFltPostOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

_Function_class_(PFLT_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS FLTAPI KphpFltPreOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltCleanupFileOp(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltInitializeFileOp(
    VOID
    );

// informer_filenc.c

typedef enum _KPH_FLT_FILE_NAME_TYPE
{
    KphFltFileNameTypeNone,
    KphFltFileNameTypeNameInfo,
    KphFltFileNameTypeContext,
    KphFltFileNameTypeFileName,
    KphFltFileNameTypeNameCache,
} KPH_FLT_FILE_NAME_TYPE, *PKPH_FLT_FILE_NAME_ITYPE;

typedef struct _KPH_FLT_FILE_NAME_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    PFILE_OBJECT FileObject;
    UNICODE_STRING FileName;
} KPH_FLT_FILE_NAME_CACHE_ENTRY, *PKPH_FLT_FILE_NAME_CACHE_ENTRY;

typedef struct _KPH_FLT_FILE_NAME
{
    //
    // N.B. PFLT_CONTEXT stores a PUNICODE_STRING
    //
    KPH_FLT_FILE_NAME_TYPE Type;
    union
    {
        PFLT_FILE_NAME_INFORMATION NameInfo;
        PFLT_CONTEXT Context;
        PUNICODE_STRING FileName;
        PKPH_FLT_FILE_NAME_CACHE_ENTRY NameCache;
    };
} KPH_FLT_FILE_NAME, *PKPH_FLT_FILE_NAME;

#define KphpFltZeroFileName(info)                                              \
    (info)->Type = KphFltFileNameTypeNone;                                     \
    (info)->NameInfo = NULL;

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetFileName(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltFileName
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltGetDestFileName(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PKPH_FLT_FILE_NAME FltDestFileName
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltReleaseFileName(
    _In_ PKPH_FLT_FILE_NAME FltFileName
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltReapFileNameCache(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltCleanupFileNameCache(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltInitializeFileNameCache(
    VOID
    );
