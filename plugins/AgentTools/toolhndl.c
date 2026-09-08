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

// What one handle really refers to, when the handle list can only give a name. ALPC is the transport
// almost every RPC call on Windows rides on, and an ALPC port handle by itself says nothing about who
// is on the other end - the kernel knows, and the System Informer driver is the only way to ask.

PCWSTR AtpAlpcPortTypeString(
    _In_ ULONG State
    )
{
    switch ((State >> 1) & 0x3)
    {
    case 1:
        return L"server_connection";
    case 2:
        return L"client_communication";
    case 3:
        return L"server_communication";
    }

    return L"unconnected";
}

// One of the three ports in a communication triple, or the port the handle itself refers to. The
// owner is the answer people come here for: it is the process on the other end.
PVOID AtpCreateAlpcPortObject(
    _In_ PKPH_ALPC_BASIC_INFORMATION Basic,
    _In_opt_ PUNICODE_STRING Name
    )
{
    static CONST ULONG portFlags[] =
    {
        ALPC_PORFLG_LPC_MODE, ALPC_PORFLG_ALLOW_IMPERSONATION, ALPC_PORFLG_ALLOW_LPC_REQUESTS,
        ALPC_PORFLG_WAITABLE_PORT, ALPC_PORFLG_ALLOW_DUP_OBJECT, ALPC_PORFLG_SYSTEM_PROCESS,
        ALPC_PORFLG_WAKE_POLICY1, ALPC_PORFLG_WAKE_POLICY2, ALPC_PORFLG_WAKE_POLICY3,
        ALPC_PORFLG_DIRECT_MESSAGE, ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE
    };
    static CONST PWSTR portFlagNames[] =
    {
        L"lpc_mode", L"allow_impersonation", L"allow_lpc_requests",
        L"waitable", L"allow_dup_object", L"system_process_only",
        L"wake_policy1", L"wake_policy2", L"wake_policy3",
        L"direct_message", L"allow_multihandle_attribute"
    };
    // The state word carries the port type in bits 1-2; every other bit is a flag.
    static CONST ULONG stateFlags[] =
    {
        0x00001, 0x00008, 0x00010, 0x00020, 0x00040, 0x00080, 0x00100, 0x00200,
        0x00400, 0x00800, 0x01000, 0x02000, 0x04000, 0x08000, 0x10000
    };
    static CONST PWSTR stateFlagNames[] =
    {
        L"initialized", L"connection_pending", L"connection_refused", L"disconnected", L"closed",
        L"no_flush_on_close", L"return_extended_info", L"waitable", L"dynamic_security",
        L"wow64_completion_list", L"lpc", L"lpc_to_lpc", L"has_completion_list",
        L"had_completion_list", L"enable_completion_list"
    };
    PVOID object;

    object = PhCreateJsonObject();

    if (Basic->OwnerProcessId)
    {
        PVOID owner = PhCreateJsonObject();
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(Basic->OwnerProcessId))
        {
            AtFillProcessIdentity(owner, processItem);
            PhDereferenceObject(processItem);
        }
        else
        {
            PhAddJsonObjectUInt64(owner, "pid", HandleToUlong(Basic->OwnerProcessId));
        }

        PhAddJsonObjectValue(object, "owner", owner);
    }
    else
    {
        AtJsonAddNull(object, "owner");
    }

    if (Name && Name->Length != 0)
    {
        PH_STRINGREF name;

        PhUnicodeStringToStringRef(Name, &name);
        AtJsonAddStringRef(object, "name", &name);
    }
    else
    {
        AtJsonAddNull(object, "name");
    }

    AtJsonAddStringZ(object, "port_type", AtpAlpcPortTypeString(Basic->State));
    AtJsonAddHex(object, "state", Basic->State);
    AtJsonAddFlagStrings(object, "state_flags", Basic->State,
        stateFlags, (CONST PWSTR*)stateFlagNames, RTL_NUMBER_OF(stateFlags));
    AtJsonAddHex(object, "flags", Basic->Flags);
    AtJsonAddFlagStrings(object, "flag_names", Basic->Flags,
        portFlags, (CONST PWSTR*)portFlagNames, RTL_NUMBER_OF(portFlags));
    PhAddJsonObjectInt64(object, "sequence_number", Basic->SequenceNo);
    AtJsonAddPointer(object, "port_context", Basic->PortContext);

    return object;
}

VOID AtpAddAlpcPeer(
    _In_ PVOID Structured,
    _In_ PCSTR Key,
    _In_ PKPH_ALPC_BASIC_INFORMATION Basic,
    _In_opt_ PUNICODE_STRING Name
    )
{
    // A port the kernel left unset is not a peer with no owner, it is not part of this connection.
    if (Basic->OwnerProcessId || (Name && Name->Length != 0))
        PhAddJsonObjectValue(Structured, Key, AtpCreateAlpcPortObject(Basic, Name));
    else
        AtJsonAddNull(Structured, Key);
}

VOID AtpGetAlpcPortInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    KPH_ALPC_BASIC_INFORMATION basicInfo;
    KPH_ALPC_COMMUNICATION_INFORMATION communicationInfo;
    PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION names = NULL;
    PVOID structured;

    // A plain read is handed no target: mcp.c resolves one only for the tiers that hold it open
    // across a confirmation. The sequence number stays optional, as it is on every other read.
    if (!NT_SUCCESS(AtResolveHandleTarget(Call->Arguments, FALSE, PROCESS_QUERY_LIMITED_INFORMATION, &target, Result)))
        return;

    if (!target.HandleTypeName || !PhEqualString2(target.HandleTypeName, L"ALPC Port", TRUE))
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_OBJECT_TYPE_MISMATCH,
            L"Handle 0x%llx in pid %lu is a %s handle, not an ALPC Port.",
            (ULONG64)(ULONG_PTR)target.HandleValue,
            HandleToUlong(target.ProcessItem->ProcessId),
            PhGetStringOrDefault(target.HandleTypeName, L"(unknown type)")
            );
        AtDeleteTarget(&target);
        return;
    }

    // Nothing here is reachable without the driver: the port objects live in kernel memory and no
    // user-mode call reports them.
    if (KsiLevel() < KphLevelMed)
    {
        AtSetToolError(
            Result,
            "failed",
            STATUS_NOT_SUPPORTED,
            L"ALPC port information comes from the System Informer driver, which is not available to this instance (access level: %s).",
            AtKphLevelString(KsiLevel())
            );
        AtSetToolHint(Result, AT_HINT_NEEDS_DRIVER);
        AtDeleteTarget(&target);
        return;
    }

    status = KphAlpcQueryInformation(
        target.ProcessHandle,
        target.HandleValue,
        KphAlpcBasicInformation,
        &basicInfo,
        sizeof(basicInfo),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Querying the ALPC port");
        AtDeleteTarget(&target);
        return;
    }

    memset(&communicationInfo, 0, sizeof(communicationInfo));

    if (!NT_SUCCESS(KphAlpcQueryInformation(
        target.ProcessHandle,
        target.HandleValue,
        KphAlpcCommunicationInformation,
        &communicationInfo,
        sizeof(communicationInfo),
        NULL
        )))
    {
        // An unconnected port has no triple; the basic information is still worth returning.
        memset(&communicationInfo, 0, sizeof(communicationInfo));
    }

    if (!NT_SUCCESS(KphAlpcQueryCommunicationsNamesInfo(
        target.ProcessHandle,
        target.HandleValue,
        &names
        )))
    {
        names = NULL;
    }

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "pid", HandleToUlong(target.ProcessItem->ProcessId));
    AtJsonAddString(structured, "process_name", target.ProcessItem->ProcessName);
    AtJsonAddPointer(structured, "handle", target.HandleValue);
    AtJsonAddString(structured, "object_name", target.HandleObjectName);

    PhAddJsonObjectValue(structured, "port", AtpCreateAlpcPortObject(&basicInfo, NULL));

    AtpAddAlpcPeer(structured, "connection_port", &communicationInfo.ConnectionPort,
        names ? &names->ConnectionPort : NULL);
    AtpAddAlpcPeer(structured, "server_communication_port", &communicationInfo.ServerCommunicationPort,
        names ? &names->ServerCommunicationPort : NULL);
    AtpAddAlpcPeer(structured, "client_communication_port", &communicationInfo.ClientCommunicationPort,
        names ? &names->ClientCommunicationPort : NULL);

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    if (names)
        PhFree(names);

    AtDeleteTarget(&target);
}

// The object behind a handle, without duplicating it. The driver can read a file object's state,
// a section's backing file or an ETW registration's GUID on behalf of another process, which is the
// only way to see inside a protected process - nothing else can even open one for PROCESS_DUP_HANDLE.
// Without the driver the handle is duplicated instead and the parts that survive that are reported,
// with source saying which route the answer came by.

typedef struct _AT_HANDLE_QUERY
{
    HANDLE ProcessHandle;   // the process that owns the handle
    HANDLE Handle;          // the handle value inside that process
    HANDLE LocalHandle;     // the same object duplicated into this process, or NULL
    BOOLEAN UseDriver;
} AT_HANDLE_QUERY, *PAT_HANDLE_QUERY;

NTSTATUS AtpQuerySectionInfo(
    _In_ PAT_HANDLE_QUERY Query,
    _In_ SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    if (Query->UseDriver)
    {
        KPH_OBJECT_INFORMATION_CLASS objectClass;

        switch (SectionInformationClass)
        {
        case SectionBasicInformation:
            objectClass = KphObjectSectionBasicInformation;
            break;
        case SectionImageInformation:
            objectClass = KphObjectSectionImageInformation;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        return KphQueryInformationObject(Query->ProcessHandle, Query->Handle, objectClass, Buffer, Length, NULL);
    }

    if (Query->LocalHandle)
        return NtQuerySection(Query->LocalHandle, SectionInformationClass, Buffer, Length, NULL);

    return STATUS_NOT_SUPPORTED;
}

// Named pipes, \Device\ConDrv\CurrentIn and \Device\VolMgrControl deadlock a file query outright, so
// both routes go through the timeout wrappers rather than the plain calls.
NTSTATUS AtpQueryFileInfo(
    _In_ PAT_HANDLE_QUERY Query,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    if (Query->UseDriver)
    {
        return PhCallKphQueryFileInformationWithTimeout(
            Query->ProcessHandle,
            Query->Handle,
            FileInformationClass,
            Buffer,
            Length,
            NULL
            );
    }

    if (Query->LocalHandle)
        return PhCallNtQueryFileInformationWithTimeout(Query->LocalHandle, FileInformationClass, Buffer, Length, NULL);

    return STATUS_NOT_SUPPORTED;
}

// A UNICODE_STRING class, whose length nothing tells you in advance.
PPH_STRING AtpQueryObjectString(
    _In_ PAT_HANDLE_QUERY Query,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass
    )
{
    NTSTATUS status;
    PPH_STRING string = NULL;
    PVOID buffer;
    ULONG bufferSize = 0x100;
    ULONG returnLength = 0;

    if (!Query->UseDriver)
        return NULL;

    buffer = PhAllocate(bufferSize);

    status = KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
        ObjectInformationClass, buffer, bufferSize, &returnLength);

    if ((status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) && returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            ObjectInformationClass, buffer, bufferSize, &returnLength);
    }

    if (NT_SUCCESS(status))
        string = PhCreateStringFromUnicodeString(buffer);

    PhFree(buffer);

    return string;
}

PCWSTR AtpDeviceTypeString(
    _In_ ULONG DeviceType
    )
{
    switch (DeviceType)
    {
    case FILE_DEVICE_DISK:
    case FILE_DEVICE_DISK_FILE_SYSTEM:
    case FILE_DEVICE_VIRTUAL_DISK:
        return L"disk";
    case FILE_DEVICE_NAMED_PIPE:
        return L"named_pipe";
    case FILE_DEVICE_NETWORK:
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
    case FILE_DEVICE_NETWORK_REDIRECTOR:
        return L"network";
    case FILE_DEVICE_CONSOLE:
        return L"console";
    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        return L"cd_rom";
    case FILE_DEVICE_SERIAL_PORT:
        return L"serial_port";
    case FILE_DEVICE_KEYBOARD:
        return L"keyboard";
    case FILE_DEVICE_MOUSE:
        return L"mouse";
    case FILE_DEVICE_SCREEN:
        return L"screen";
    case FILE_DEVICE_MAILSLOT:
        return L"mailslot";
    case FILE_DEVICE_DFS:
    case FILE_DEVICE_DFS_FILE_SYSTEM:
    case FILE_DEVICE_DFS_VOLUME:
        return L"dfs";
    }

    return NULL;
}

VOID AtpAddFileHandleDetails(
    _In_ PVOID Structured,
    _In_ PAT_HANDLE_QUERY Query
    )
{
    static CONST ULONG modeFlags[] =
    {
        FILE_WRITE_THROUGH, FILE_SEQUENTIAL_ONLY, FILE_NO_INTERMEDIATE_BUFFERING,
        FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_NONALERT, FILE_DELETE_ON_CLOSE
    };
    static CONST PWSTR modeNames[] =
    {
        L"write_through", L"sequential_only", L"no_intermediate_buffering",
        L"synchronous_io_alert", L"synchronous_io_nonalert", L"delete_on_close"
    };
    KPH_FILE_OBJECT_INFORMATION fileObjectInfo;
    FILE_STANDARD_INFORMATION standardInfo;
    FILE_MODE_INFORMATION modeInfo;
    FILE_POSITION_INFORMATION positionInfo;
    PVOID details;

    details = PhCreateJsonObject();

    // The file object's own state: who has it open for what, and whether it is on its way out. Only
    // the driver can read this, because it is the kernel structure and not the file behind it.
    if (Query->UseDriver && NT_SUCCESS(KphQueryInformationObject(
        Query->ProcessHandle,
        Query->Handle,
        KphObjectFileObjectInformation,
        &fileObjectInfo,
        sizeof(fileObjectInfo),
        NULL
        )))
    {
        PhAddJsonObjectBoolean(details, "delete_pending", !!fileObjectInfo.DeletePending);
        PhAddJsonObjectBoolean(details, "read_access", !!fileObjectInfo.ReadAccess);
        PhAddJsonObjectBoolean(details, "write_access", !!fileObjectInfo.WriteAccess);
        PhAddJsonObjectBoolean(details, "delete_access", !!fileObjectInfo.DeleteAccess);
        PhAddJsonObjectBoolean(details, "shared_read", !!fileObjectInfo.SharedRead);
        PhAddJsonObjectBoolean(details, "shared_write", !!fileObjectInfo.SharedWrite);
        PhAddJsonObjectBoolean(details, "shared_delete", !!fileObjectInfo.SharedDelete);
        PhAddJsonObjectBoolean(details, "has_active_transaction", !!fileObjectInfo.HasActiveTransaction);
        PhAddJsonObjectBoolean(details, "is_ignoring_sharing", !!fileObjectInfo.IsIgnoringSharing);
        PhAddJsonObjectUInt64(details, "user_writable_references", fileObjectInfo.UserWritableReferences);
        PhAddJsonObjectInt64(details, "waiters", fileObjectInfo.Waiters);
        PhAddJsonObjectInt64(details, "busy", fileObjectInfo.Busy);
        AtJsonAddStringZ(details, "device_type", AtpDeviceTypeString(fileObjectInfo.Device.Type));
        AtJsonAddHex(details, "device_type_value", fileObjectInfo.Device.Type);

        if (fileObjectInfo.Vpb.VolumeLabelLength != 0)
        {
            PH_STRINGREF label;

            label.Buffer = fileObjectInfo.Vpb.VolumeLabel;
            label.Length = min(fileObjectInfo.Vpb.VolumeLabelLength, sizeof(fileObjectInfo.Vpb.VolumeLabel));
            AtJsonAddStringRef(details, "volume_label", &label);
        }
        else
        {
            AtJsonAddNull(details, "volume_label");
        }

        AtJsonAddHex(details, "volume_serial_number", fileObjectInfo.Vpb.SerialNumber);
    }
    else
    {
        AtJsonAddNull(details, "delete_pending");
        AtJsonAddNull(details, "device_type");
        AtJsonAddNull(details, "volume_label");
    }

    if (NT_SUCCESS(AtpQueryFileInfo(Query, FileStandardInformation, &standardInfo, sizeof(standardInfo))))
    {
        PhAddJsonObjectUInt64(details, "size", standardInfo.EndOfFile.QuadPart);
        PhAddJsonObjectUInt64(details, "allocation_size", standardInfo.AllocationSize.QuadPart);
        PhAddJsonObjectBoolean(details, "directory", !!standardInfo.Directory);
        PhAddJsonObjectUInt64(details, "link_count", standardInfo.NumberOfLinks);
    }

    if (NT_SUCCESS(AtpQueryFileInfo(Query, FileModeInformation, &modeInfo, sizeof(modeInfo))))
    {
        AtJsonAddHex(details, "mode", modeInfo.Mode);
        AtJsonAddFlagStrings(details, "mode_flags", modeInfo.Mode,
            modeFlags, (CONST PWSTR*)modeNames, RTL_NUMBER_OF(modeFlags));
    }

    if (NT_SUCCESS(AtpQueryFileInfo(Query, FilePositionInformation, &positionInfo, sizeof(positionInfo))))
        PhAddJsonObjectUInt64(details, "position", positionInfo.CurrentByteOffset.QuadPart);

    // Which driver owns the device this file lives on: the answer to "what is actually servicing
    // this handle", which a file name does not give you when a filter or a redirector is involved.
    if (Query->UseDriver)
    {
        HANDLE driverHandle;

        if (NT_SUCCESS(KphQueryInformationObject(
            Query->ProcessHandle,
            Query->Handle,
            KphObjectFileObjectDriver,
            &driverHandle,
            sizeof(HANDLE),
            NULL
            )))
        {
            PVOID driver = PhCreateJsonObject();
            PPH_STRING name;

            if (NT_SUCCESS(PhGetDriverName(driverHandle, &name)))
            {
                AtJsonAddString(driver, "name", name);
                PhDereferenceObject(name);
            }
            else
            {
                AtJsonAddNull(driver, "name");
            }

            if (NT_SUCCESS(PhGetDriverImageFileName(driverHandle, &name)))
            {
                AtJsonAddString(driver, "image_file_name", name);
                PhDereferenceObject(name);
            }
            else
            {
                AtJsonAddNull(driver, "image_file_name");
            }

            PhAddJsonObjectValue(details, "driver", driver);
            NtClose(driverHandle);
        }
        else
        {
            AtJsonAddNull(details, "driver");
        }
    }
    else
    {
        AtJsonAddNull(details, "driver");
    }

    PhAddJsonObjectValue(Structured, "file", details);
}

VOID AtpAddSectionHandleDetails(
    _In_ PVOID Structured,
    _In_ PAT_HANDLE_QUERY Query
    )
{
    SECTION_BASIC_INFORMATION basicInfo;
    SECTION_IMAGE_INFORMATION imageInfo;
    PPH_STRING fileName;
    BOOLEAN haveImage;

    if (!NT_SUCCESS(AtpQuerySectionInfo(Query, SectionBasicInformation, &basicInfo, sizeof(basicInfo))))
        return;

    haveImage = FlagOn(basicInfo.AllocationAttributes, SEC_IMAGE) &&
        NT_SUCCESS(AtpQuerySectionInfo(Query, SectionImageInformation, &imageInfo, sizeof(imageInfo)));

    fileName = AtpQueryObjectString(Query, KphObjectSectionFileName);

    if (!fileName && Query->LocalHandle)
        PhGetSectionFileName(Query->LocalHandle, &fileName);

    AtAddSectionInfo(Structured, &basicInfo, haveImage ? &imageInfo : NULL, fileName);
    PhClearReference(&fileName);
}

VOID AtpAddProcessHandleDetails(
    _In_ PVOID Structured,
    _In_ PAT_HANDLE_QUERY Query
    )
{
    PROCESS_BASIC_INFORMATION basicInfo;
    KERNEL_USER_TIMES times;
    PVOID details;
    PPH_STRING imageFileName = NULL;
    NTSTATUS status;

    if (Query->UseDriver)
    {
        status = KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            KphObjectProcessBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
    }
    else if (Query->LocalHandle)
    {
        status = PhGetProcessBasicInformation(Query->LocalHandle, &basicInfo);
    }
    else
    {
        return;
    }

    if (!NT_SUCCESS(status))
        return;

    details = PhCreateJsonObject();

    // The whole point: the process a handle refers to, which the handle list cannot name.
    PhAddJsonObjectUInt64(details, "pid", HandleToUlong(basicInfo.UniqueProcessId));

    if (Query->UseDriver)
        imageFileName = AtpQueryObjectString(Query, KphObjectProcessImageFileName);
    else if (Query->LocalHandle)
        PhGetProcessImageFileName(Query->LocalHandle, &imageFileName);

    AtJsonAddString(details, "image_file_name", imageFileName);
    AtJsonAddWin32FileName(details, "image_path", imageFileName);
    PhClearReference(&imageFileName);

    if (Query->UseDriver)
    {
        status = KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            KphObjectProcessTimes, &times, sizeof(times), NULL);
    }
    else
    {
        status = NtQueryInformationProcess(Query->LocalHandle, ProcessTimes, &times, sizeof(times), NULL);
    }

    if (NT_SUCCESS(status))
    {
        AtJsonAddTime(details, "create_time", &times.CreateTime);

        if (times.ExitTime.QuadPart != 0)
            AtJsonAddTime(details, "exit_time", &times.ExitTime);
        else
            AtJsonAddNull(details, "exit_time");
    }

    PhAddJsonObjectValue(Structured, "process", details);
}

VOID AtpAddThreadHandleDetails(
    _In_ PVOID Structured,
    _In_ PAT_HANDLE_QUERY Query
    )
{
    THREAD_BASIC_INFORMATION basicInfo;
    KERNEL_USER_TIMES times;
    PPH_STRING name = NULL;
    PVOID details;
    ULONG terminated;
    NTSTATUS status;

    if (Query->UseDriver)
    {
        status = KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            KphObjectThreadBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
    }
    else if (Query->LocalHandle)
    {
        status = PhGetThreadBasicInformation(Query->LocalHandle, &basicInfo);
    }
    else
    {
        return;
    }

    if (!NT_SUCCESS(status))
        return;

    details = PhCreateJsonObject();
    PhAddJsonObjectUInt64(details, "tid", HandleToUlong(basicInfo.ClientId.UniqueThread));
    PhAddJsonObjectUInt64(details, "pid", HandleToUlong(basicInfo.ClientId.UniqueProcess));

    if (Query->UseDriver)
    {
        name = AtpQueryObjectString(Query, KphObjectThreadNameInformation);

        if (NT_SUCCESS(KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            KphObjectThreadIsTerminated, &terminated, sizeof(terminated), NULL)))
        {
            PhAddJsonObjectBoolean(details, "terminated", !!terminated);
        }
        else
        {
            AtJsonAddNull(details, "terminated");
        }

        if (NT_SUCCESS(KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
            KphObjectThreadTimes, &times, sizeof(times), NULL)))
        {
            AtJsonAddTime(details, "create_time", &times.CreateTime);
        }
    }
    else
    {
        BOOLEAN terminatedFlag = FALSE;

        PhGetThreadName(Query->LocalHandle, &name);

        if (NT_SUCCESS(PhGetThreadIsTerminated(Query->LocalHandle, &terminatedFlag)))
            PhAddJsonObjectBoolean(details, "terminated", !!terminatedFlag);
        else
            AtJsonAddNull(details, "terminated");

        if (NT_SUCCESS(PhGetThreadTimes(Query->LocalHandle, &times)))
            AtJsonAddTime(details, "create_time", &times.CreateTime);
    }

    AtJsonAddString(details, "name", name);
    PhClearReference(&name);

    PhAddJsonObjectValue(Structured, "thread", details);
}

VOID AtpAddEtwRegistrationDetails(
    _In_ PVOID Structured,
    _In_ PAT_HANDLE_QUERY Query
    )
{
    KPH_ETWREG_BASIC_INFORMATION basicInfo;
    PVOID details;
    PPH_STRING guid;

    // No user-mode call reports the provider behind an ETW registration handle.
    if (!Query->UseDriver)
        return;

    if (!NT_SUCCESS(KphQueryInformationObject(Query->ProcessHandle, Query->Handle,
        KphObjectEtwRegBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
    {
        return;
    }

    details = PhCreateJsonObject();
    guid = PhFormatGuid(&basicInfo.Guid);
    AtJsonAddString(details, "provider_guid", guid);
    PhClearReference(&guid);
    PhAddJsonObjectUInt64(details, "session_id", basicInfo.SessionId);
    PhAddJsonObjectValue(Structured, "etw_registration", details);
}

VOID AtpGetHandleDetails(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST ULONG attributeFlags[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
    static CONST PWSTR attributeNames[] =
    {
        L"new_object", L"kernel_object", L"kernel_only_access", L"exclusive_object",
        L"permanent_object", L"default_security_quota", L"single_handle_entry", L"deleted_inline"
    };
    AT_HANDLE_QUERY query;
    KPH_OBJECT_ATTRIBUTES_INFORMATION attributesInfo;
    HANDLE dupProcessHandle = NULL;
    PPH_STRING objectName = NULL;
    PVOID structured;

    memset(&query, 0, sizeof(AT_HANDLE_QUERY));
    query.ProcessHandle = Target->ProcessHandle;
    query.Handle = Target->HandleValue;
    query.UseDriver = KsiLevel() >= KphLevelMed;

    // Without the driver the object has to come here to be read. That needs PROCESS_DUP_HANDLE,
    // which a protected process will never give, so the driver path is the only one that reaches
    // those - the answer says which route it took rather than pretending they are the same.
    if (!query.UseDriver)
    {
        if (NT_SUCCESS(PhOpenProcess(&dupProcessHandle, PROCESS_DUP_HANDLE, Target->ProcessItem->ProcessId)))
        {
            NtDuplicateObject(
                dupProcessHandle,
                Target->HandleValue,
                NtCurrentProcess(),
                &query.LocalHandle,
                0,
                0,
                DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
                );
        }

        if (!query.LocalHandle)
        {
            AtSetToolError(
                Result,
                "failed",
                STATUS_NOT_SUPPORTED,
                L"Handle 0x%llx in pid %lu could not be duplicated and the System Informer driver is not available to read it in place (access level: %s).",
                (ULONG64)(ULONG_PTR)Target->HandleValue,
                HandleToUlong(Target->ProcessItem->ProcessId),
                AtKphLevelString(KsiLevel())
                );
            AtSetToolHint(Result, AT_HINT_NEEDS_DRIVER);

            if (dupProcessHandle)
                NtClose(dupProcessHandle);

            return;
        }
    }

    // The driver names the object during target resolution; the duplicate path could not, because
    // resolution deliberately asks for no more than PROCESS_QUERY_LIMITED_INFORMATION so that a
    // protected process still resolves. Now that the object is here, it can be named.
    if (!Target->HandleObjectName && query.LocalHandle)
    {
        // The type index is what selects the best-name routine for a process, thread or key handle;
        // without it the name is only ever whatever the object itself reports.
        PhGetHandleInformation(NtCurrentProcess(), query.LocalHandle, Target->HandleTypeIndex,
            NULL, NULL, NULL, &objectName);
    }

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "pid", HandleToUlong(Target->ProcessItem->ProcessId));
    AtJsonAddString(structured, "process_name", Target->ProcessItem->ProcessName);
    AtJsonAddPointer(structured, "handle", Target->HandleValue);
    AtJsonAddString(structured, "type", Target->HandleTypeName);
    AtJsonAddString(structured, "object_name", Target->HandleObjectName ? Target->HandleObjectName : objectName);
    AtJsonAddStringZ(structured, "source", query.UseDriver ? L"driver" : L"duplicated_handle");

    if (query.UseDriver && NT_SUCCESS(KphQueryInformationObject(
        query.ProcessHandle,
        query.Handle,
        KphObjectAttributesInformation,
        &attributesInfo,
        sizeof(attributesInfo),
        NULL
        )))
    {
        AtJsonAddFlagStrings(structured, "attributes", attributesInfo.Flags,
            attributeFlags, (CONST PWSTR*)attributeNames, RTL_NUMBER_OF(attributeFlags));
    }
    else
    {
        AtJsonAddNull(structured, "attributes");
    }

    if (Target->HandleTypeName)
    {
        if (PhEqualString2(Target->HandleTypeName, L"File", TRUE))
            AtpAddFileHandleDetails(structured, &query);
        else if (PhEqualString2(Target->HandleTypeName, L"Section", TRUE))
            AtpAddSectionHandleDetails(structured, &query);
        else if (PhEqualString2(Target->HandleTypeName, L"Process", TRUE))
            AtpAddProcessHandleDetails(structured, &query);
        else if (PhEqualString2(Target->HandleTypeName, L"Thread", TRUE))
            AtpAddThreadHandleDetails(structured, &query);
        else if (PhEqualString2(Target->HandleTypeName, L"EtwRegistration", TRUE))
            AtpAddEtwRegistrationDetails(structured, &query);
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    PhClearReference(&objectName);

    if (query.LocalHandle)
        NtClose(query.LocalHandle);
    if (dupProcessHandle)
        NtClose(dupProcessHandle);
}

VOID AtHandleInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetAlpcPortInfo:
        AtpGetAlpcPortInfo(Call, Result);
        break;
    case AtActionGetHandleDetails:
        AtpGetHandleDetails(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"Unhandled tool.");
        break;
    }
}
