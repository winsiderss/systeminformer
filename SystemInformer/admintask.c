/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <appsup.h>
#include <taskschd.h>

DEFINE_GUID(CLSID_TaskScheduler, 0x0f87369f, 0xa4e5, 0x4cfc, 0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd);
DEFINE_GUID(IID_ITaskService, 0x2FABA4C7, 0x4DA9, 0x4013, 0x96, 0x97, 0x20, 0xCC, 0x3F, 0xD4, 0x0F, 0x85);
DEFINE_GUID(IID_ITaskSettings2, 0x2C05C3F0, 0x6EED, 0x4C05, 0xA1, 0x5F, 0xED, 0x7D, 0x7A, 0x98, 0xA3, 0x69);
DEFINE_GUID(IID_ILogonTrigger, 0x72DADE38, 0xFAE4, 0x4b3e, 0xBA, 0xF4, 0x5D, 0x00, 0x9A, 0xF0, 0x2B, 0x1C);
DEFINE_GUID(IID_IExecAction, 0x4C3D624D, 0xfd6b, 0x49A3, 0xB9, 0xB7, 0x09, 0xCB, 0x3C, 0xD3, 0xF0, 0x47);

HRESULT PhCreateAdminTask(
    _In_ PPH_STRINGREF TaskName,
    _In_ PPH_STRINGREF FileName
    )
{
    static PH_STRINGREF taskTimeLimit = PH_STRINGREF_INIT(L"PT0S");
    HRESULT status;
    BSTR taskNameString = NULL;
    BSTR taskFileNameString = NULL;
    BSTR taskFolderString = NULL;
    BSTR taskTimeLimitString = NULL;
    VARIANT empty = { VT_EMPTY };
    ITaskService* taskService = NULL;
    ITaskFolder* taskFolder = NULL;
    ITaskDefinition* taskDefinition = NULL;
    ITaskSettings* taskSettings = NULL;
    ITaskSettings2* taskSettings2 = NULL;
    ITriggerCollection* taskTriggerCollection = NULL;
    ITrigger* taskTrigger = NULL;
    ILogonTrigger* taskLogonTrigger = NULL;
    IRegisteredTask* taskRegisteredTask = NULL;
    IPrincipal* taskPrincipal = NULL;
    IActionCollection* taskActionCollection = NULL;
    IAction* taskAction = NULL;
    IExecAction* taskExecAction = NULL;

    status = PhGetClassObject(
        L"taskschd.dll",
        &CLSID_TaskScheduler,
        &IID_ITaskService,
        &taskService
        );

    if (FAILED(status))
        goto CleanupExit;

    taskNameString = SysAllocStringLen(TaskName->Buffer, (UINT32)TaskName->Length);
    taskFileNameString = SysAllocStringLen(FileName->Buffer, (UINT32)FileName->Length);
    taskFolderString = SysAllocStringLen(PhNtPathSeperatorString.Buffer, (UINT32)PhNtPathSeperatorString.Length);
    taskTimeLimitString = SysAllocStringLen(taskTimeLimit.Buffer, (UINT32)taskTimeLimit.Length);

    status = ITaskService_Connect(
        taskService,
        empty,
        empty,
        empty,
        empty
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskService_GetFolder(
        taskService,
        taskFolderString,
        &taskFolder
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskService_NewTask(
        taskService,
        0,
        &taskDefinition
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskDefinition_get_Settings(
        taskDefinition,
        &taskSettings
        );

    if (FAILED(status))
        goto CleanupExit;

    ITaskSettings_put_Compatibility(taskSettings, TASK_COMPATIBILITY_V2_1);
    ITaskSettings_put_StartWhenAvailable(taskSettings, VARIANT_TRUE);
    ITaskSettings_put_DisallowStartIfOnBatteries(taskSettings, VARIANT_FALSE);
    ITaskSettings_put_StopIfGoingOnBatteries(taskSettings, VARIANT_FALSE);
    ITaskSettings_put_ExecutionTimeLimit(taskSettings, taskTimeLimitString);
    ITaskSettings_put_Priority(taskSettings, 1);

    if (SUCCEEDED(ITaskSettings_QueryInterface(
        taskSettings,
        &IID_ITaskSettings2,
        &taskSettings2
        )))
    {
        ITaskSettings2_put_UseUnifiedSchedulingEngine(taskSettings2, VARIANT_TRUE);
        ITaskSettings2_put_DisallowStartOnRemoteAppSession(taskSettings2, VARIANT_TRUE);
        ITaskSettings2_Release(taskSettings2);
    }

    //status = ITaskDefinition_get_Triggers(
    //    taskDefinition,
    //    &taskTriggerCollection
    //    );
    //
    //if (FAILED(status))
    //    goto CleanupExit;
    //
    //status = ITriggerCollection_Create(
    //    taskTriggerCollection,
    //    TASK_TRIGGER_LOGON,
    //    &taskTrigger
    //    );
    //
    //if (FAILED(status))
    //    goto CleanupExit;
    //
    //status = ITrigger_QueryInterface(
    //    taskTrigger,
    //    &IID_ILogonTrigger,
    //    &taskLogonTrigger
    //    );
    //
    //if (FAILED(status))
    //    goto CleanupExit;
    //
    //ILogonTrigger_put_Id(taskLogonTrigger, L"LogonTriggerId");
    //ILogonTrigger_put_UserId(taskLogonTrigger, PhGetString(PhGetTokenUserString(PhGetOwnTokenAttributes().TokenHandle, TRUE)));

    status = ITaskDefinition_get_Principal(
        taskDefinition,
        &taskPrincipal
        );

    if (FAILED(status))
        goto CleanupExit;

    IPrincipal_put_RunLevel(taskPrincipal, TASK_RUNLEVEL_HIGHEST);
    IPrincipal_put_LogonType(taskPrincipal, TASK_LOGON_INTERACTIVE_TOKEN);

    status = ITaskDefinition_get_Actions(
        taskDefinition,
        &taskActionCollection
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IActionCollection_Create(
        taskActionCollection,
        TASK_ACTION_EXEC,
        &taskAction
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IAction_QueryInterface(
        taskAction,
        &IID_IExecAction,
        &taskExecAction
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IExecAction_put_Path(
        taskExecAction,
        taskFileNameString
        );

    if (FAILED(status))
        goto CleanupExit;

    ITaskFolder_DeleteTask(
        taskFolder,
        taskNameString,
        0
        );

    status = ITaskFolder_RegisterTaskDefinition(
        taskFolder,
        taskNameString,
        taskDefinition,
        TASK_CREATE_OR_UPDATE,
        empty,
        empty,
        TASK_LOGON_INTERACTIVE_TOKEN,
        empty,
        &taskRegisteredTask
        );

CleanupExit:

    if (taskRegisteredTask)
        IRegisteredTask_Release(taskRegisteredTask);
    if (taskActionCollection)
        IActionCollection_Release(taskActionCollection);
    if (taskPrincipal)
        IPrincipal_Release(taskPrincipal);
    if (taskLogonTrigger)
        ILogonTrigger_Release(taskLogonTrigger);
    if (taskTrigger)
        ITrigger_Release(taskTrigger);
    if (taskTriggerCollection)
        ITriggerCollection_Release(taskTriggerCollection);
    if (taskSettings)
        ITaskSettings_Release(taskSettings);
    if (taskDefinition)
        ITaskDefinition_Release(taskDefinition);
    if (taskFolder)
        ITaskFolder_Release(taskFolder);
    if (taskService)
        ITaskService_Release(taskService);
    if (taskTimeLimitString)
        SysFreeString(taskTimeLimitString);
    if (taskNameString)
        SysFreeString(taskNameString);
    if (taskFileNameString)
        SysFreeString(taskFileNameString);
    if (taskFolderString)
        SysFreeString(taskFolderString);

    return status;
}

HRESULT PhDeleteAdminTask(
    _In_ PPH_STRINGREF TaskName
    )
{
    HRESULT status;
    BSTR taskNameString = NULL;
    BSTR taskFolderString = NULL;
    VARIANT empty = { VT_EMPTY };
    ITaskService* taskService = NULL;
    ITaskFolder* taskFolder = NULL;

    status = PhGetClassObject(
        L"taskschd.dll",
        &CLSID_TaskScheduler,
        &IID_ITaskService,
        &taskService
        );

    if (FAILED(status))
        goto CleanupExit;

    taskNameString = SysAllocStringLen(TaskName->Buffer, (UINT32)TaskName->Length);
    taskFolderString = SysAllocStringLen(PhNtPathSeperatorString.Buffer, (UINT32)PhNtPathSeperatorString.Length);

    status = ITaskService_Connect(
        taskService,
        empty,
        empty,
        empty,
        empty
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskService_GetFolder(
        taskService,
        taskFolderString,
        &taskFolder
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskFolder_DeleteTask(
        taskFolder,
        taskNameString,
        0
        );

CleanupExit:

    if (taskFolder)
        ITaskFolder_Release(taskFolder);
    if (taskService)
        ITaskService_Release(taskService);
    if (taskFolderString)
        SysFreeString(taskFolderString);
    if (taskNameString)
        SysFreeString(taskNameString);

    return status;
}

HRESULT PhRunAsAdminTask(
    _In_ PPH_STRINGREF TaskName
    )
{
    HRESULT status;
    BSTR taskNameString = NULL;
    BSTR taskFolderString = NULL;
    VARIANT empty = { VT_EMPTY };
    ITaskService* taskService = NULL;
    ITaskFolder* taskFolder = NULL;
    IRegisteredTask* taskRegisteredTask = NULL;
    IRunningTask* taskRunningTask = NULL;

    status = PhGetClassObject(
        L"taskschd.dll",
        &CLSID_TaskScheduler,
        &IID_ITaskService,
        &taskService
        );

    if (FAILED(status))
        goto CleanupExit;

    taskNameString = SysAllocStringLen(TaskName->Buffer, (UINT32)TaskName->Length);
    taskFolderString = SysAllocStringLen(PhNtPathSeperatorString.Buffer, (UINT32)PhNtPathSeperatorString.Length);

    status = ITaskService_Connect(
        taskService,
        empty,
        empty,
        empty,
        empty
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskService_GetFolder(
        taskService,
        taskFolderString,
        &taskFolder
        );

    if (FAILED(status))
        goto CleanupExit;

    status = ITaskFolder_GetTask(
        taskFolder,
        taskNameString,
        &taskRegisteredTask
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IRegisteredTask_RunEx(
        taskRegisteredTask,
        empty,
        TASK_RUN_AS_SELF,
        0,
        NULL,
        &taskRunningTask
        );

CleanupExit:

    if (taskRunningTask)
        IRunningTask_Release(taskRunningTask);
    if (taskRegisteredTask)
        IRegisteredTask_Release(taskRegisteredTask);
    if (taskFolder)
        ITaskFolder_Release(taskFolder);
    if (taskService)
        ITaskService_Release(taskService);
    if (taskFolderString)
        SysFreeString(taskFolderString);
    if (taskNameString)
        SysFreeString(taskNameString);

    return status;
}
