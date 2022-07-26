/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include <ph.h>

NTSTATUS InitializeAttributeList(
    _In_ STARTUPINFOEX* StartupInfo
    )
{
#define DEFAULT_MITIGATION_POLICY_FLAGS \
    (PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON)
#define DEFAULT_MITIGATION_POLICY_FLAGS2 \
    (PROCESS_CREATION_MITIGATION_POLICY2_LOADER_INTEGRITY_CONTINUITY_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY2_STRICT_CONTROL_FLOW_GUARD_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY2_MODULE_TAMPERING_PROTECTION_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY2_RESTRICT_INDIRECT_BRANCH_PREDICTION_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY2_SPECULATIVE_STORE_BYPASS_DISABLE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY2_CET_USER_SHADOW_STACKS_ALWAYS_ON)

    SIZE_T attributeListLength = 0;

    if (!InitializeProcThreadAttributeList(NULL, 1, 0, &attributeListLength) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return PhGetLastWin32ErrorAsNtStatus();

    StartupInfo->lpAttributeList = PhAllocate(attributeListLength);

    if (!InitializeProcThreadAttributeList(StartupInfo->lpAttributeList, 1, 0, &attributeListLength))
        return PhGetLastWin32ErrorAsNtStatus();

    if (!UpdateProcThreadAttribute(
        StartupInfo->lpAttributeList,
        0,
        PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
        &(ULONG64[2]) { DEFAULT_MITIGATION_POLICY_FLAGS, DEFAULT_MITIGATION_POLICY_FLAGS2 },
        sizeof(ULONG64[2]),
        NULL,
        NULL
        ))
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    return STATUS_SUCCESS;
}

NTSTATUS DestroyAttributeList(
    _In_ STARTUPINFOEX* StartupInfo
    )
{
    DeleteProcThreadAttributeList(StartupInfo->lpAttributeList);
    return STATUS_SUCCESS;
}

NTSTATUS InitializeJobObject(
    _Out_ PHANDLE JobObjectHandle
    )
{
    static JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedInfo =
    {
        0, 0, 0, 0,
        JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_ACTIVE_PROCESS,
        0, 0,
        1,
        0,
        0,
        0
    };
    static JOBOBJECT_BASIC_UI_RESTRICTIONS basicInfo =
    {
        JOB_OBJECT_UILIMIT_HANDLES | JOB_OBJECT_UILIMIT_GLOBALATOMS | JOB_OBJECT_UILIMIT_DESKTOP
    };
    NTSTATUS status;
    HANDLE jobObjectHandle;

    status = NtCreateJobObject(
        &jobObjectHandle,
        JOB_OBJECT_ALL_ACCESS,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        NtSetInformationJobObject(
            jobObjectHandle,
            JobObjectExtendedLimitInformation,
            &extendedInfo,
            sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)
            );

        NtSetInformationJobObject(
            jobObjectHandle,
            JobObjectBasicUIRestrictions,
            &basicInfo,
            sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS)
            );

        *JobObjectHandle = jobObjectHandle;
    }

    return status;
}

NTSTATUS InitializeFileName(
    _Out_ PPH_STRING* FileName
    )
{
#ifdef _DEBUG
    PH_STRINGREF fileNameStringRef = PH_STRINGREF_INIT(L"\\..\\..\\..\\..\\bin\\Debug64\\SystemInformer.exe");
#else
    PH_STRINGREF fileNameStringRef = PH_STRINGREF_INIT(L"SystemInformer.exe");
#endif
    PPH_STRING fileName;

    fileName = PhGetApplicationDirectoryFileNameWin32(&fileNameStringRef);
    PhMoveReference(&fileName, PhGetFullPath(fileName->Buffer, NULL));

    *FileName = fileName;
    return STATUS_SUCCESS;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ PWSTR CmdLine,
    _In_ INT CmdShow
    )
{
    HANDLE processHandle;
    HANDLE jobObjectHandle;
    PPH_STRING fileName;
    PROCESS_INFORMATION processInfo = { 0 };
    STARTUPINFOEX info = { sizeof(STARTUPINFOEX) };

    if (!NT_SUCCESS(PhInitializePhLib(L"CustomStartTool", Instance)))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(InitializeAttributeList(&info)))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(InitializeJobObject(&jobObjectHandle)))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(InitializeFileName(&fileName)))
        return EXIT_FAILURE;

    if (NT_SUCCESS(PhCreateProcessWin32Ex(
        NULL,
        PhGetString(fileName),
        NULL,
        NULL,
        &info.StartupInfo,
        PH_CREATE_PROCESS_SUSPENDED | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB,
        NULL,
        NULL,
        &processHandle,
        NULL
        )))
    {
        if (jobObjectHandle)
        {
            NtAssignProcessToJobObject(jobObjectHandle, processHandle);
            NtClose(jobObjectHandle);
        }

        NtResumeProcess(processHandle);
        NtClose(processHandle);
    }

    if (processInfo.hThread)
    {
        NtClose(processInfo.hThread);
    }

    DestroyAttributeList(&info);
    PhDereferenceObject(fileName);

    return EXIT_SUCCESS;
}
