/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2026
 *
 */

#include <ph.h>

NTSTATUS InitializeAttributeList(
    _Out_ STARTUPINFOEX* StartupInfo,
    _In_ HANDLE JobObjectHandle
    )
{
    static ULONG64 mitigationFlags[] =
    {
        (PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
         //PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON),
        (PROCESS_CREATION_MITIGATION_POLICY2_LOADER_INTEGRITY_CONTINUITY_ALWAYS_ON |
         //PROCESS_CREATION_MITIGATION_POLICY2_STRICT_CONTROL_FLOW_GUARD_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_MODULE_TAMPERING_PROTECTION_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_RESTRICT_INDIRECT_BRANCH_PREDICTION_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_SPECULATIVE_STORE_BYPASS_DISABLE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_CET_USER_SHADOW_STACKS_ALWAYS_ON)
    };
    NTSTATUS status;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;

    RtlZeroMemory(StartupInfo, sizeof(STARTUPINFOEX));
    StartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

    status = PhInitializeProcThreadAttributeList(&attributeList, 2);

    if (!NT_SUCCESS(status))
        return status;

    StartupInfo->lpAttributeList = attributeList;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_JOB_LIST,
        &JobObjectHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
        mitigationFlags,
        sizeof(mitigationFlags)
        );

    return status;
}

NTSTATUS DestroyAttributeList(
    _In_ STARTUPINFOEX* StartupInfo
    )
{
    PhDeleteProcThreadAttributeList(StartupInfo->lpAttributeList);
    return STATUS_SUCCESS;
}

NTSTATUS InitializeJobObject(
    _Out_ PHANDLE JobObjectHandle
    )
{
    return PhCreateConfiguredJobObject(JobObjectHandle);
}

NTSTATUS InitializeFileName(
    _Out_ PPH_STRING* FileName
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    PPH_STRING fullPath;

#ifdef _DEBUG
    fileName = PhGetApplicationDirectoryFileNameZ(L"\\..\\..\\..\\..\\bin\\Debug64\\SystemInformer.exe", FALSE);
#else
    fileName = PhGetApplicationDirectoryFileNameZ(L"SystemInformer.exe", FALSE);
#endif

    status = PhGetFullPath(
        PhGetString(fileName),
        &fullPath,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *FileName = fullPath;
    }

    PhDereferenceObject(fileName);

    return status;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ PWSTR CmdLine,
    _In_ INT CmdShow
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE jobObjectHandle;
    PPH_STRING fileName;
    STARTUPINFOEX info;

    if (!NT_SUCCESS(status = PhInitializePhLib(L"CustomStartTool")))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(status = InitializeJobObject(&jobObjectHandle)))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(status = InitializeAttributeList(&info, jobObjectHandle)))
        return EXIT_FAILURE;
    if (!NT_SUCCESS(status = InitializeFileName(&fileName)))
        return EXIT_FAILURE;

    if (NT_SUCCESS(status = PhCreateProcessWin32Ex(
        NULL,
        PhGetString(fileName),
        NULL,
        NULL,
        &info.StartupInfo,
        PH_CREATE_PROCESS_SUSPENDED | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO,
        NULL,
        NULL,
        &processHandle,
        NULL
        )))
    {
        NtResumeProcess(processHandle);
        NtClose(processHandle);
    }

    if (jobObjectHandle)
        NtClose(jobObjectHandle);

    DestroyAttributeList(&info);
    PhDereferenceObject(fileName);

    return EXIT_SUCCESS;
}
