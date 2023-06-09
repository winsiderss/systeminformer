/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_HNDLINFO_H
#define _PH_HNDLINFO_H

EXTERN_C_START

#define MAX_OBJECT_TYPE_NUMBER 257

extern BOOLEAN PhEnableProcessHandlePnPDeviceNameSupport;

typedef PPH_STRING (NTAPI *PPH_GET_CLIENT_ID_NAME)(
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
PPH_GET_CLIENT_ID_NAME
NTAPI
PhSetHandleClientIdFunction(
    _In_ PPH_GET_CLIENT_ID_NAME GetClientIdName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryObjectName(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* ObjectName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryObjectBasicInformation(
    _In_ HANDLE Handle,
    _Out_ POBJECT_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetEtwPublisherName(
    _In_ PGUID Guid
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatNativeKeyName(
    _In_ PPH_STRING Name
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSectionFileName(
    _In_ HANDLE SectionHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
_Callback_ PPH_STRING
NTAPI
PhStdGetClientIdName(
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
PPH_STRING
NTAPI
PhStdGetClientIdNameEx(
    _In_ PCLIENT_ID ClientId,
    _In_opt_ PPH_STRING ProcessName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetHandleInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetHandleInformationEx(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Reserved_ ULONG Flags,
    _Out_opt_ PNTSTATUS SubStatus,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName,
    _Reserved_ PVOID *ExtraInformation
    );

#define PH_FIRST_OBJECT_TYPE(ObjectTypes) \
    PTR_ADD_OFFSET((ObjectTypes), ALIGN_UP(sizeof(OBJECT_TYPES_INFORMATION), ULONG_PTR))

#define PH_NEXT_OBJECT_TYPE(ObjectType) \
    PTR_ADD_OFFSET((ObjectType), sizeof(OBJECT_TYPE_INFORMATION) + \
    ALIGN_UP((ObjectType)->TypeName.MaximumLength, ULONG_PTR))

PHLIBAPI
NTSTATUS
NTAPI
PhEnumObjectTypes(
    _Out_ POBJECT_TYPES_INFORMATION *ObjectTypes
    );

PHLIBAPI
ULONG
NTAPI
PhGetObjectTypeNumber(
    _In_ PPH_STRINGREF TypeName
    );

FORCEINLINE
ULONG
NTAPI
PhGetObjectTypeNumberZ(
    _In_ PWSTR TypeName
    )
{
    PH_STRINGREF typeName;

    PhInitializeStringRef(&typeName, TypeName);

    return PhGetObjectTypeNumber(&typeName);
}

PHLIBAPI
PPH_STRING
NTAPI
PhGetObjectTypeName(
    _In_ ULONG TypeIndex
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallWithTimeout(
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_opt_ PLARGE_INTEGER AcquireTimeout,
    _In_ PLARGE_INTEGER CallTimeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallNtQueryObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallNtQuerySecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallNtSetSecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallNtQueryFileInformationWithTimeout(
    _In_ HANDLE Handle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_opt_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCallKphQueryFileInformationWithTimeout(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_opt_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength
    );

EXTERN_C_END

#endif
