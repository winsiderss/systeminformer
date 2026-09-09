/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <secedit.h>
#include <aclapi.h>
#include <sddl.h>

typedef enum _AT_SECURITY_KIND
{
    AtSecurityKindFile,
    AtSecurityKindRegistry,
    AtSecurityKindService,
    AtSecurityKindProcess,
    AtSecurityKindThread,
    AtSecurityKindDescriptor
} AT_SECURITY_KIND;

PCWSTR AtpSecurityKindString(
    _In_ AT_SECURITY_KIND Kind
    )
{
    switch (Kind)
    {
    case AtSecurityKindFile:
        return L"file";
    case AtSecurityKindRegistry:
        return L"registry";
    case AtSecurityKindService:
        return L"service";
    case AtSecurityKindProcess:
        return L"process";
    case AtSecurityKindThread:
        return L"thread";
    case AtSecurityKindDescriptor:
        return L"descriptor";
    }

    return NULL;
}

PCWSTR AtpSecurityAccessType(
    _In_ AT_SECURITY_KIND Kind
    )
{
    switch (Kind)
    {
    case AtSecurityKindFile:
        return L"File";
    case AtSecurityKindRegistry:
        return L"Key";
    case AtSecurityKindService:
        return L"Service";
    case AtSecurityKindProcess:
        return L"Process";
    case AtSecurityKindThread:
        return L"Thread";
    }

    return NULL;
}

PCWSTR AtpAceTypeString(
    _In_ UCHAR Type
    )
{
    switch (Type)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        return L"allowed";
    case ACCESS_DENIED_ACE_TYPE:
        return L"denied";
    case SYSTEM_AUDIT_ACE_TYPE:
        return L"audit";
    case SYSTEM_ALARM_ACE_TYPE:
        return L"alarm";
    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        return L"allowed_object";
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        return L"denied_object";
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        return L"audit_object";
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        return L"alarm_object";
    case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
        return L"allowed_callback";
    case ACCESS_DENIED_CALLBACK_ACE_TYPE:
        return L"denied_callback";
    case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
        return L"allowed_callback_object";
    case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
        return L"denied_callback_object";
    case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:
        return L"audit_callback";
    case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
        return L"mandatory_label";
    case SYSTEM_RESOURCE_ATTRIBUTE_ACE_TYPE:
        return L"resource_attribute";
    case SYSTEM_SCOPED_POLICY_ID_ACE_TYPE:
        return L"scoped_policy_id";
    case SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE:
        return L"process_trust_label";
    case SYSTEM_ACCESS_FILTER_ACE_TYPE:
        return L"access_filter";
    }

    return NULL;
}

PSID AtpAceSid(
    _In_ PACE_HEADER Ace
    )
{
    switch (Ace->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
    case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
    case ACCESS_DENIED_CALLBACK_ACE_TYPE:
    case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:
    case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
    case SYSTEM_RESOURCE_ATTRIBUTE_ACE_TYPE:
    case SYSTEM_SCOPED_POLICY_ID_ACE_TYPE:
    case SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE:
    case SYSTEM_ACCESS_FILTER_ACE_TYPE:
        return (PSID)&((PACCESS_ALLOWED_ACE)Ace)->SidStart;
    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
    case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
        {
            PACCESS_ALLOWED_OBJECT_ACE ace = (PACCESS_ALLOWED_OBJECT_ACE)Ace;
            PVOID sid = &ace->ObjectType;

            if (ace->Flags & ACE_OBJECT_TYPE_PRESENT)
                sid = PTR_ADD_OFFSET(sid, sizeof(GUID));
            if (ace->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                sid = PTR_ADD_OFFSET(sid, sizeof(GUID));

            return sid;
        }
    }

    return NULL;
}

VOID AtpAddTrustee(
    _In_ PVOID Object,
    _In_opt_ PSID Sid
    )
{
    PPH_STRING string;

    if (!Sid || !RtlValidSid(Sid))
    {
        AtJsonAddNull(Object, "sid");
        AtJsonAddNull(Object, "name");
        return;
    }

    if (string = PhSidToStringSid(Sid))
    {
        AtJsonAddString(Object, "sid", string);
        PhDereferenceObject(string);
    }
    else
    {
        AtJsonAddNull(Object, "sid");
    }

    // A SID that cannot be named is not an error: an account from a domain that cannot be reached,
    // or one that has been deleted, looks exactly like this.
    if (string = PhGetSidFullName(Sid, TRUE, NULL))
    {
        AtJsonAddString(Object, "name", string);
        PhDereferenceObject(string);
    }
    else
    {
        AtJsonAddNull(Object, "name");
    }
}

VOID AtpAddAcl(
    _In_ PVOID Structured,
    _In_ PCSTR Key,
    _In_opt_ PACL Acl,
    _In_ BOOLEAN Present,
    _In_opt_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    )
{
    static CONST ULONG aceFlags[] =
    {
        OBJECT_INHERIT_ACE, CONTAINER_INHERIT_ACE, NO_PROPAGATE_INHERIT_ACE,
        INHERIT_ONLY_ACE, INHERITED_ACE, SUCCESSFUL_ACCESS_ACE_FLAG, FAILED_ACCESS_ACE_FLAG
    };
    static CONST PWSTR aceNames[] =
    {
        L"object_inherit", L"container_inherit", L"no_propagate_inherit",
        L"inherit_only", L"inherited", L"successful_access", L"failed_access"
    };
    PVOID array;
    ULONG i;

    // A DACL that is absent and one that is empty are opposite answers: absent means everyone is
    // allowed everything, empty means nobody is allowed anything.
    if (!Present || !Acl)
    {
        AtJsonAddNull(Structured, Key);
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < Acl->AceCount; i++)
    {
        PACE_HEADER ace;
        PVOID entry;
        PSID sid;
        ACCESS_MASK mask;

        if (!NT_SUCCESS(RtlGetAce(Acl, i, &ace)))
            break;

        entry = PhCreateJsonObject();
        AtJsonAddStringZ(entry, "type", AtpAceTypeString(ace->AceType));
        PhAddJsonObjectUInt64(entry, "type_value", ace->AceType);

        sid = AtpAceSid(ace);
        AtpAddTrustee(entry, sid);

        // Every entry this knows the shape of starts with a mask in the same place.
        if (sid)
        {
            mask = ((PACCESS_ALLOWED_ACE)ace)->Mask;
            AtJsonAddHex(entry, "access_mask", mask);

            if (AccessEntries)
            {
                PPH_STRING access;

                if (access = PhGetAccessString(mask, AccessEntries, NumberOfAccessEntries))
                {
                    AtJsonAddString(entry, "access", access);
                    PhDereferenceObject(access);
                }
                else
                {
                    AtJsonAddNull(entry, "access");
                }
            }
            else
            {
                AtJsonAddNull(entry, "access");
            }
        }
        else
        {
            AtJsonAddNull(entry, "access_mask");
            AtJsonAddNull(entry, "access");
        }

        AtJsonAddHex(entry, "flags", ace->AceFlags);
        AtJsonAddFlagStrings(entry, "flag_names", ace->AceFlags,
            aceFlags, (CONST PWSTR*)aceNames, RTL_NUMBER_OF(aceFlags));
        PhAddJsonObjectBoolean(entry, "inherited", !!(ace->AceFlags & INHERITED_ACE));

        PhAddJsonArrayObject(array, entry);
    }

    PhAddJsonObjectValue(Structured, Key, array);
}

VOID AtpAddIntegrity(
    _In_ PVOID Structured,
    _In_opt_ PACL Sacl,
    _In_ BOOLEAN Present
    )
{
    ULONG i;

    if (Present && Sacl)
    {
        for (i = 0; i < Sacl->AceCount; i++)
        {
            PACE_HEADER ace;
            PVOID entry;
            PSID sid;
            PPH_STRING string;

            if (!NT_SUCCESS(RtlGetAce(Sacl, i, &ace)))
                break;

            if (ace->AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
                continue;

            if (!(sid = AtpAceSid(ace)) || !RtlValidSid(sid))
                continue;

            entry = PhCreateJsonObject();
            AtpAddTrustee(entry, sid);

            // The policy is what the label does, not what it is: no-write-up is the ordinary case
            // and no-read-up is what keeps a low process from reading the object at all.
            AtJsonAddHex(entry, "policy", ((PSYSTEM_MANDATORY_LABEL_ACE)ace)->Mask);
            PhAddJsonObjectBoolean(entry, "no_write_up",
                !!(((PSYSTEM_MANDATORY_LABEL_ACE)ace)->Mask & SYSTEM_MANDATORY_LABEL_NO_WRITE_UP));
            PhAddJsonObjectBoolean(entry, "no_read_up",
                !!(((PSYSTEM_MANDATORY_LABEL_ACE)ace)->Mask & SYSTEM_MANDATORY_LABEL_NO_READ_UP));
            PhAddJsonObjectBoolean(entry, "no_execute_up",
                !!(((PSYSTEM_MANDATORY_LABEL_ACE)ace)->Mask & SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP));

            if (string = PhGetSidFullName(sid, TRUE, NULL))
                PhDereferenceObject(string);

            PhAddJsonObjectValue(Structured, "integrity", entry);
            return;
        }
    }

    // No label is the normal case and means medium integrity, which is why it is null rather than
    // an invented level. The caller tells that apart from a label nobody could read by way of
    // integrity_readable, which is false in the second case.
    AtJsonAddNull(Structured, "integrity");
}

NTSTATUS AtpOpenSecurityObject(
    _In_ AT_SECURITY_KIND Kind,
    _In_opt_ PPH_STRING Path,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _Out_ PHANDLE Handle
    )
{
    NTSTATUS status;
    HANDLE handle = NULL;

    *Handle = NULL;

    switch (Kind)
    {
    case AtSecurityKindFile:
        // No FILE_DIRECTORY_FILE or FILE_NON_DIRECTORY_FILE, so a directory opens the same way a
        // file does.
        status = PhCreateFileWin32(
            &handle,
            PhGetString(Path),
            READ_CONTROL | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_SYNCHRONOUS_IO_NONALERT
            );
        break;
    case AtSecurityKindRegistry:
        {
            PPH_STRING subKey;
            HANDLE root;
            PCWSTR nativeRoot;

            AtParseRegistryPath(Path, &root, &subKey, &nativeRoot);
            status = PhOpenKey(&handle, READ_CONTROL, root, &subKey->sr, 0);
            PhDereferenceObject(subKey);
        }
        break;
    case AtSecurityKindService:
        status = PhOpenService((PSC_HANDLE)&handle, READ_CONTROL, PhGetString(Path));
        break;
    case AtSecurityKindProcess:
        status = PhOpenProcess(&handle, READ_CONTROL, ProcessId);
        break;
    case AtSecurityKindThread:
        status = PhOpenThread(&handle, READ_CONTROL, ThreadId);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (NT_SUCCESS(status))
        *Handle = handle;

    return status;
}

VOID AtpCloseSecurityObject(
    _In_ AT_SECURITY_KIND Kind,
    _In_ HANDLE Handle
    )
{
    // A service handle comes from the service manager and is not an object handle.
    if (Kind == AtSecurityKindService)
        PhCloseServiceHandle(Handle);
    else
        NtClose(Handle);
}

VOID AtpGetObjectSecurity(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST PH_STRINGREF registryPrefix = PH_STRINGREF_INIT(L"\\Registry\\");
    NTSTATUS status;
    AT_SECURITY_KIND kind;
    PPH_STRING path = NULL;
    PPH_STRING serviceName;
    PPH_STRING typeName;
    PPH_STRING sddlArgument;
    PPH_STRING sddl = NULL;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    BOOLEAN localDescriptor = FALSE;
    BOOLEAN labelQueried = TRUE;
    PPH_ACCESS_ENTRY accessEntries = NULL;
    ULONG numberOfAccessEntries = 0;
    HANDLE handle = NULL;
    ULONG64 processId = 0;
    ULONG64 threadId = 0;
    PVOID structured;
    PVOID entry;
    PSID sid;
    PACL acl;
    BOOLEAN present;
    BOOLEAN defaulted;
    SECURITY_DESCRIPTOR_CONTROL control;
    ULONG revision;

    path = AtGetArgumentString(Call->Arguments, "path");
    serviceName = AtGetArgumentString(Call->Arguments, "service_name");
    sddlArgument = AtGetArgumentString(Call->Arguments, "sddl");
    typeName = AtGetArgumentString(Call->Arguments, "type");

    if (sddlArgument)
    {
        kind = AtSecurityKindDescriptor;
    }
    else if (serviceName)
    {
        kind = AtSecurityKindService;
        PhMoveReference(&path, PhReferenceObject(serviceName));
    }
    else if (AtGetArgumentUInt64(Call->Arguments, "tid", &threadId))
    {
        kind = AtSecurityKindThread;
    }
    else if (AtGetArgumentUInt64(Call->Arguments, "pid", &processId))
    {
        kind = AtSecurityKindProcess;
    }
    else if (path)
    {
        // A registry path is named by its hive prefix or by the native root; anything else with a
        // path is a file, which is what a caller means by a path far more often.
        if (typeName && PhEqualString2(typeName, L"registry", TRUE))
            kind = AtSecurityKindRegistry;
        else if (typeName && PhEqualString2(typeName, L"file", TRUE))
            kind = AtSecurityKindFile;
        else if (PhStartsWithStringRef(&path->sr, &registryPrefix, TRUE) ||
            PhStartsWithString2(path, L"HK", TRUE))
            kind = AtSecurityKindRegistry;
        else
            kind = AtSecurityKindFile;
    }
    else
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"One of path, service_name, pid, tid or sddl is required.");
        goto CleanupExit;
    }

    if (kind == AtSecurityKindDescriptor)
    {
        PVOID localSecurityDescriptor;

        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            PhGetString(sddlArgument),
            SDDL_REVISION,
            &localSecurityDescriptor,
            NULL
            ))
        {
            AtSetToolError(Result, "invalid_arguments", PhGetLastWin32ErrorAsNtStatus(),
                L"The security descriptor could not be read as SDDL.");
            goto CleanupExit;
        }

        securityDescriptor = localSecurityDescriptor;
        localDescriptor = TRUE;
        PhSetReference(&sddl, sddlArgument);
    }
    else
    {
        status = AtpOpenSecurityObject(kind, path, (HANDLE)(ULONG_PTR)processId, (HANDLE)(ULONG_PTR)threadId, &handle);

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Opening the object");
            goto CleanupExit;
        }

        // The label lives in the SACL, and asking for it needs no privilege the way the audit part
        // of a SACL does; asking for both together fails, so the label is asked for on its own.
        // SE_SERVICE refuses it outright, so a service is the one kind whose label is never read
        // and must not be reported as simply absent.
        if (kind == AtSecurityKindService)
        {
            labelQueried = FALSE;

            status = PhGetSeObjectSecurity(
                handle,
                SE_SERVICE,
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                &securityDescriptor
                );
        }
        else
        {
            status = PhGetObjectSecurity(
                handle,
                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
                &securityDescriptor
                );

            if (!NT_SUCCESS(status))
            {
                status = PhGetObjectSecurity(
                    handle,
                    OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                    &securityDescriptor
                    );
            }
        }

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Reading the security descriptor");
            goto CleanupExit;
        }
    }

    // The type decides what a mask means. For a descriptor handed in, the caller says which type it
    // came from, and without that the masks are reported as numbers rather than as the wrong names.
    if (typeName && kind == AtSecurityKindDescriptor)
        PhGetAccessEntries(PhGetString(typeName), &accessEntries, &numberOfAccessEntries);
    else if (AtpSecurityAccessType(kind))
        PhGetAccessEntries(AtpSecurityAccessType(kind), &accessEntries, &numberOfAccessEntries);

    structured = PhCreateJsonObject();
    AtJsonAddStringZ(structured, "kind", AtpSecurityKindString(kind));
    AtJsonAddString(structured, "path", path);

    if (kind == AtSecurityKindProcess || kind == AtSecurityKindThread)
        PhAddJsonObjectUInt64(structured, "pid", kind == AtSecurityKindThread ? threadId : processId);
    else
        AtJsonAddNull(structured, "pid");

    if (accessEntries)
        AtJsonAddString(structured, "access_type", typeName && kind == AtSecurityKindDescriptor ?
            typeName : PhaCreateString(AtpSecurityAccessType(kind)));
    else
        AtJsonAddNull(structured, "access_type");

    entry = PhCreateJsonObject();

    if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(securityDescriptor, &sid, &defaulted)))
        AtpAddTrustee(entry, sid);
    else
        AtpAddTrustee(entry, NULL);

    PhAddJsonObjectValue(structured, "owner", entry);

    entry = PhCreateJsonObject();

    if (NT_SUCCESS(RtlGetGroupSecurityDescriptor(securityDescriptor, &sid, &defaulted)))
        AtpAddTrustee(entry, sid);
    else
        AtpAddTrustee(entry, NULL);

    PhAddJsonObjectValue(structured, "group", entry);

    present = FALSE;
    acl = NULL;

    if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(securityDescriptor, &present, &acl, &defaulted)))
    {
        AtpAddAcl(structured, "dacl", acl, present, accessEntries, numberOfAccessEntries);
        PhAddJsonObjectBoolean(structured, "dacl_present", !!present);
    }
    else
    {
        // A query that failed is not a DACL that is absent, and absent is the permissive answer:
        // it means everyone is allowed everything.
        AtJsonAddNull(structured, "dacl");
        AtJsonAddNull(structured, "dacl_present");
    }

    present = FALSE;
    acl = NULL;

    // A null integrity means the object carries no label, which is medium - so it can only be
    // reported once the label has actually been looked for and found absent.
    if (!labelQueried)
    {
        AtJsonAddNull(structured, "integrity");
        PhAddJsonObjectBoolean(structured, "integrity_readable", FALSE);
    }
    else if (NT_SUCCESS(RtlGetSaclSecurityDescriptor(securityDescriptor, &present, &acl, &defaulted)))
    {
        AtpAddIntegrity(structured, acl, present);
        PhAddJsonObjectBoolean(structured, "integrity_readable", TRUE);
    }
    else
    {
        AtJsonAddNull(structured, "integrity");
        PhAddJsonObjectBoolean(structured, "integrity_readable", FALSE);
    }

    if (NT_SUCCESS(RtlGetControlSecurityDescriptor(securityDescriptor, &control, &revision)))
    {
        AtJsonAddHex(structured, "control", control);
        PhAddJsonObjectBoolean(structured, "dacl_protected", !!(control & SE_DACL_PROTECTED));
        PhAddJsonObjectBoolean(structured, "dacl_auto_inherited", !!(control & SE_DACL_AUTO_INHERITED));
    }
    else
    {
        AtJsonAddNull(structured, "control");
        AtJsonAddNull(structured, "dacl_protected");
        AtJsonAddNull(structured, "dacl_auto_inherited");
    }

    if (!sddl && handle)
        PhGetObjectSecurityDescriptorAsString(handle, &sddl);

    AtJsonAddString(structured, "sddl", sddl);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

CleanupExit:
    if (accessEntries)
        PhFree(accessEntries);
    if (securityDescriptor)
    {
        if (localDescriptor)
            LocalFree(securityDescriptor);
        else
            PhFree(securityDescriptor);
    }
    if (handle)
        AtpCloseSecurityObject(kind, handle);

    PhClearReference(&sddl);
    PhClearReference(&typeName);
    PhClearReference(&sddlArgument);
    PhClearReference(&serviceName);
    PhClearReference(&path);
}

VOID AtSecurityInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetObjectSecurity:
        AtpGetObjectSecurity(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
