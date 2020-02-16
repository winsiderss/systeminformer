/*
 * Process Hacker -
 *   main program
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2020 dmex
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

#include <phapp.h>

#include <shlobj.h>

#include <colorbox.h>
#include <hexedit.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <lsasup.h>

#include <extmgri.h>
#include <hndlprv.h>
#include <mainwnd.h>
#include <memprv.h>
#include <modprv.h>
#include <netprv.h>
#include <phsvc.h>
#include <procprp.h>
#include <procprv.h>
#include <settings.h>
#include <srvprv.h>
#include <thrdprv.h>

LONG PhMainMessageLoop(
    VOID
    );

VOID PhActivatePreviousInstance(
    VOID
    );

VOID PhInitializeCommonControls(
    VOID
    );

VOID PhInitializeKph(
    VOID
    );

BOOLEAN PhInitializeAppSystem(
    VOID
    );

VOID PhpInitializeSettings(
    VOID
    );

VOID PhpProcessStartupParameters(
    VOID
    );

VOID PhpEnablePrivileges(
    VOID
    );

BOOLEAN PhInitializeExceptionPolicy(
    VOID
    );

BOOLEAN PhInitializeNamespacePolicy(
    VOID
    );

BOOLEAN PhInitializeMitigationPolicy(
    VOID
    );

BOOLEAN PhPluginsEnabled = FALSE;
PPH_STRING PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS PhStartupParameters;

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;

static PPH_LIST DialogList = NULL;
static PPH_LIST FilterList = NULL;
static PH_AUTO_POOL BaseAutoPool;

INT WINAPI wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ PWSTR CmdLine,
    _In_ INT CmdShow
    )
{
    LONG result;
#ifdef DEBUG
    PHP_BASE_THREAD_DBG dbg;
#endif

    if (!NT_SUCCESS(PhInitializePhLibEx(L"Process Hacker", ULONG_MAX, Instance, 0, 0)))
        return 1;
    if (!PhInitializeExceptionPolicy())
        return 1;
    if (!PhInitializeNamespacePolicy())
        return 1;
    if (!PhInitializeMitigationPolicy())
        return 1;
    //if (!PhInitializeRestartPolicy())
    //    return 1;

    PhpProcessStartupParameters();
    PhpEnablePrivileges();

    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return 1;

    if (PhStartupParameters.RunAsServiceMode)
    {
        RtlExitUserProcess(PhRunAsServiceStart(PhStartupParameters.RunAsServiceMode));
    }

    if (PhStartupParameters.CommandMode &&
        PhStartupParameters.CommandType &&
        PhStartupParameters.CommandAction)
    {
        RtlExitUserProcess(PhCommandModeStart());
    }

    PhSettingsInitialization();
    PhpInitializeSettings();

    if (PhGetIntegerSetting(L"AllowOnlyOneInstance") &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.CommandMode &&
        !PhStartupParameters.PhSvc)
    {
        PhActivatePreviousInstance();
    }

    if (PhGetIntegerSetting(L"EnableStartAsAdmin") &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.CommandMode &&
        !PhStartupParameters.PhSvc)
    {
        if (!PhGetOwnTokenAttributes().Elevated)
        {
            if (SUCCEEDED(PhRunAsAdminTask(L"ProcessHackerTaskAdmin")))
            {
                RtlExitUserProcess(STATUS_SUCCESS);
            }
        }
    }

    if (PhGetIntegerSetting(L"EnableKph") &&
        !PhStartupParameters.NoKph &&
        !PhStartupParameters.CommandMode &&
        !PhIsExecutingInWow64()
        )
    {
        PhInitializeKph();
    }

#ifdef DEBUG
    dbg.ClientId = NtCurrentTeb()->ClientId;
    dbg.StartAddress = wWinMain;
    dbg.Parameter = NULL;
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    TlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    PhInitializeAutoPool(&BaseAutoPool);

    PhInitializeAppSystem();
    PhInitializeCallbacks();
    PhInitializeCommonControls();

    PhEmInitialization();
    PhGuiSupportInitialization();
    PhTreeNewInitialization();
    PhGraphControlInitialization();
    PhHexEditInitialization();
    PhColorBoxInitialization();

    if (PhStartupParameters.ShowOptions)
    {
        PhShowOptionsDialog(PhStartupParameters.WindowHandle);
        RtlExitUserProcess(STATUS_SUCCESS);
    }

    if (PhPluginsEnabled && !PhStartupParameters.NoPlugins)
    {
        PhLoadPlugins();
    }

#ifndef DEBUG
    if (WindowsVersion >= WINDOWS_10)
    {
        PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

        // Note: The PhInitializeMitigationPolicy function enables the other mitigation policies.
        // However, we can only enable the ProcessSignaturePolicy after loading plugins.
        policyInfo.Policy = ProcessSignaturePolicy;
        policyInfo.SignaturePolicy.Flags = 0;
        policyInfo.SignaturePolicy.MicrosoftSignedOnly = TRUE;

        NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    }
#endif

    if (PhStartupParameters.PhSvc)
    {
        MSG message;

        // Turn the feedback cursor off.
        PostMessage(NULL, WM_NULL, 0, 0);
        GetMessage(&message, NULL, 0, 0);

        RtlExitUserProcess(PhSvcMain(NULL, NULL));
    }

#ifndef DEBUG
    if (PhIsExecutingInWow64())
    {
        PhShowWarning(
            NULL,
            L"You are attempting to run the 32-bit version of Process Hacker on 64-bit Windows. "
            L"Most features will not work correctly.\n\n"
            L"Please run the 64-bit version of Process Hacker instead."
            );
        RtlExitUserProcess(STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT);
    }
#endif

    // Set the default priority.
    {
        PROCESS_PRIORITY_CLASS priorityClass;

        priorityClass.Foreground = FALSE;
        priorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;

        if (PhStartupParameters.PriorityClass != 0)
            priorityClass.PriorityClass = (UCHAR)PhStartupParameters.PriorityClass;

        PhSetProcessPriority(NtCurrentProcess(), priorityClass);
    }

    if (!PhMainWndInitialization(CmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

    PhDrainAutoPool(&BaseAutoPool);

    result = PhMainMessageLoop();
    RtlExitUserProcess(result);
}

LONG PhMainMessageLoop(
    VOID
    )
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND_ACCEL));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        BOOLEAN processed = FALSE;
        ULONG i;

        if (result == -1)
            return 1;

        if (FilterList)
        {
            for (i = 0; i < FilterList->Count; i++)
            {
                PPH_MESSAGE_LOOP_FILTER_ENTRY entry = FilterList->Items[i];

                if (entry->Filter(&message, entry->Context))
                {
                    processed = TRUE;
                    break;
                }
            }
        }

        if (!processed)
        {
            if (
                message.hwnd == PhMainWndHandle ||
                IsChild(PhMainWndHandle, message.hwnd)
                )
            {
                if (TranslateAccelerator(PhMainWndHandle, acceleratorTable, &message))
                    processed = TRUE;
            }

            if (DialogList)
            {
                for (i = 0; i < DialogList->Count; i++)
                {
                    if (IsDialogMessage((HWND)DialogList->Items[i], &message))
                    {
                        processed = TRUE;
                        break;
                    }
                }
            }
        }

        if (!processed)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&BaseAutoPool);
    }

    return (LONG)message.wParam;
}

VOID PhRegisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    if (!DialogList)
        DialogList = PhCreateList(2);

    PhAddItemList(DialogList, (PVOID)DialogWindowHandle);
}

VOID PhUnregisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    ULONG indexOfDialog;

    if (!DialogList)
        return;

    indexOfDialog = PhFindItemList(DialogList, (PVOID)DialogWindowHandle);

    if (indexOfDialog != -1)
        PhRemoveItemList(DialogList, indexOfDialog);
}

PPH_MESSAGE_LOOP_FILTER_ENTRY PhRegisterMessageLoopFilter(
    _In_ PPH_MESSAGE_LOOP_FILTER Filter,
    _In_opt_ PVOID Context
    )
{
    PPH_MESSAGE_LOOP_FILTER_ENTRY entry;

    if (!FilterList)
        FilterList = PhCreateList(2);

    entry = PhAllocate(sizeof(PH_MESSAGE_LOOP_FILTER_ENTRY));
    entry->Filter = Filter;
    entry->Context = Context;
    PhAddItemList(FilterList, entry);

    return entry;
}

VOID PhUnregisterMessageLoopFilter(
    _In_ PPH_MESSAGE_LOOP_FILTER_ENTRY FilterEntry
    )
{
    ULONG indexOfFilter;

    if (!FilterList)
        return;

    indexOfFilter = PhFindItemList(FilterList, FilterEntry);

    if (indexOfFilter != -1)
        PhRemoveItemList(FilterList, indexOfFilter);

    PhFree(FilterEntry);
}

static BOOLEAN NTAPI PhpPreviousInstancesCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    static PH_STRINGREF objectNameSr = PH_STRINGREF_INIT(L"PhMutant_");
    HANDLE objectHandle;
    UNICODE_STRING objectNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    MUTANT_OWNER_INFORMATION objectInfo;

    if (!PhStartsWithStringRef(Name, &objectNameSr, FALSE))
        return TRUE;
    if (!PhStringRefToUnicodeString(Name, &objectNameUs))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectNameUs,
        OBJ_CASE_INSENSITIVE,
        PhGetNamespaceHandle(),
        NULL
        );

    if (!NT_SUCCESS(NtOpenMutant(
        &objectHandle,
        MUTANT_QUERY_STATE,
        &objectAttributes
        )))
    {
        return TRUE;
    }

    if (NT_SUCCESS(PhGetMutantOwnerInformation(
        objectHandle,
        &objectInfo
        )))
    {
        HWND hwnd;
        HANDLE processHandle = NULL;
        HANDLE tokenHandle = NULL;
        PTOKEN_USER tokenUser = NULL;
        ULONG attempts = 50;

        if (objectInfo.ClientId.UniqueProcess == NtCurrentProcessId())
            goto CleanupExit;
        if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, objectInfo.ClientId.UniqueProcess)))
            goto CleanupExit;
        if (!NT_SUCCESS(PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle)))
            goto CleanupExit;
        if (!NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
            goto CleanupExit;
        if (!RtlEqualSid(tokenUser->User.Sid, PhGetOwnTokenAttributes().TokenSid))
            goto CleanupExit;

        // Try to locate the window a few times because some users reported that it might not yet have been created. (dmex)
        do
        {
            if (hwnd = PhGetProcessMainWindowEx(
                objectInfo.ClientId.UniqueProcess,
                processHandle,
                FALSE
                ))
            {
                break;
            }

            PhDelayExecution(100);
        } while (--attempts != 0);

        if (hwnd)
        {
            ULONG_PTR result;

            SendMessageTimeout(hwnd, WM_PH_ACTIVATE, PhStartupParameters.SelectPid, 0, SMTO_BLOCK, 5000, &result);

            if (result == PH_ACTIVATE_REPLY)
            {
                SetForegroundWindow(hwnd);
                RtlExitUserProcess(STATUS_SUCCESS);
            }
        }

    CleanupExit:
        if (tokenUser) PhFree(tokenUser);
        if (tokenHandle) NtClose(tokenHandle);
        if (processHandle) NtClose(processHandle);
    }

    NtClose(objectHandle);
    return TRUE;
}

VOID PhActivatePreviousInstance(
    VOID
    )
{
    PhEnumDirectoryObjects(PhGetNamespaceHandle(), PhpPreviousInstancesCallback, NULL);
}

VOID PhInitializeCommonControls(
    VOID
    )
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC =
        ICC_LISTVIEW_CLASSES |
        ICC_TREEVIEW_CLASSES |
        ICC_BAR_CLASSES |
        ICC_TAB_CLASSES |
        ICC_PROGRESS_CLASS |
        ICC_COOL_CLASSES |
        ICC_STANDARD_CLASSES |
        ICC_LINK_CLASS
        ;

    InitCommonControlsEx(&icex);
}

VOID PhInitializeFont(
    VOID
    )
{
    NONCLIENTMETRICS metrics = { sizeof(metrics) };

    if (PhApplicationFont)
        DeleteObject(PhApplicationFont);

    if (
        !(PhApplicationFont = PhCreateFont(L"Microsoft Sans Serif", 8, FW_NORMAL)) &&
        !(PhApplicationFont = PhCreateFont(L"Tahoma", 8, FW_NORMAL))
        )
    {
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
            PhApplicationFont = CreateFontIndirect(&metrics.lfMessageFont);
        else
            PhApplicationFont = NULL;
    }
}

BOOLEAN PhInitializeRestartPolicy(
    VOID
    )
{
#ifndef DEBUG
    PH_STRINGREF commandLineSr;
    PH_STRINGREF fileNameSr;
    PH_STRINGREF argumentsSr;
    PPH_STRING argumentsString = NULL;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandLineSr);

    if (!PhParseCommandLineFuzzy(&commandLineSr, &fileNameSr, &argumentsSr, NULL))
        return FALSE;

    if (argumentsSr.Length)
    {
        static PH_STRINGREF commandlinePart = PH_STRINGREF_INIT(L"-nomp");

        if (PhEndsWithStringRef(&argumentsSr, &commandlinePart, FALSE))
            PhTrimStringRef(&argumentsSr, &commandlinePart, PH_TRIM_END_ONLY);

        argumentsString = PhCreateString2(&argumentsSr);
    }

    // MSDN: Do not include the file name in the command line.
    RegisterApplicationRestart(PhGetString(argumentsString), 0);

    if (argumentsString)
        PhDereferenceObject(argumentsString);
#endif
    return TRUE;
}

#ifndef DEBUG
#include <symprv.h>
#include <minidumpapiset.h>

static ULONG CALLBACK PhpUnhandledExceptionCallback(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    )
{
    PPH_STRING errorMessage;
    INT result;
    PPH_STRING message;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    TASKDIALOG_BUTTON buttons[2];

    if (NT_NTWIN32(ExceptionInfo->ExceptionRecord->ExceptionCode))
        errorMessage = PhGetStatusMessage(0, WIN32_FROM_NTSTATUS(ExceptionInfo->ExceptionRecord->ExceptionCode));
    else
        errorMessage = PhGetStatusMessage(ExceptionInfo->ExceptionRecord->ExceptionCode, 0);

    message = PhFormatString(
        L"Error code: 0x%08X (%s)",
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        PhGetStringOrEmpty(errorMessage)
        );

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_ERROR_ICON;
    config.pszMainInstruction = L"Process Hacker has crashed :(";
    config.pszContent = message->Buffer;

    buttons[0].nButtonID = IDYES;
    buttons[0].pszButtonText = L"Minidump";
    buttons[1].nButtonID = IDRETRY;
    buttons[1].pszButtonText = L"Restart";

    config.cButtons = RTL_NUMBER_OF(buttons);
    config.pButtons = buttons;
    config.nDefaultButton = IDCLOSE;

    if (TaskDialogIndirect(
        &config,
        &result,
        NULL,
        NULL
        ) == S_OK)
    {
        switch (result)
        {
        case IDRETRY:
            {
                PhShellProcessHacker(
                    NULL,
                    NULL,
                    SW_SHOW,
                    0,
                    PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                    0,
                    NULL
                    );
            }
            break;
        case IDYES:
            {
                static PH_STRINGREF dumpFilePath = PH_STRINGREF_INIT(L"%USERPROFILE%\\Desktop\\");
                HANDLE fileHandle;
                PPH_STRING dumpDirectory;
                PPH_STRING dumpFileName;
                WCHAR alphastring[16] = L"";

                dumpDirectory = PhExpandEnvironmentStrings(&dumpFilePath);
                PhGenerateRandomAlphaString(alphastring, RTL_NUMBER_OF(alphastring));

                dumpFileName = PhConcatStrings(
                    4,
                    PhGetString(dumpDirectory),
                    L"\\ProcessHacker_",
                    alphastring,
                    L"_DumpFile.dmp"
                    );

                if (NT_SUCCESS(PhCreateFileWin32(
                    &fileHandle,
                    dumpFileName->Buffer,
                    FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;

                    exceptionInfo.ThreadId = HandleToUlong(NtCurrentThreadId());
                    exceptionInfo.ExceptionPointers = ExceptionInfo;
                    exceptionInfo.ClientPointers = FALSE;

                    PhWriteMiniDumpProcess(
                        NtCurrentProcess(),
                        NtCurrentProcessId(),
                        fileHandle,
                        MiniDumpNormal,
                        &exceptionInfo,
                        NULL,
                        NULL
                        );

                    NtClose(fileHandle);
                }

                PhDereferenceObject(dumpFileName);
                PhDereferenceObject(dumpDirectory);
            }
            break;
        }
    }

    RtlExitUserProcess(ExceptionInfo->ExceptionRecord->ExceptionCode);

    PhDereferenceObject(message);
    PhDereferenceObject(errorMessage);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

BOOLEAN PhInitializeExceptionPolicy(
    VOID
    )
{
#ifndef DEBUG
    ULONG errorMode;

    if (NT_SUCCESS(PhGetProcessErrorMode(NtCurrentProcess(), &errorMode)))
    {
        errorMode &= ~(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        PhSetProcessErrorMode(NtCurrentProcess(), errorMode);
    }

    // NOTE: We really shouldn't be using this function since it can be
    // preempted by the Win32 SetUnhandledExceptionFilter function. (dmex)
    RtlSetUnhandledExceptionFilter(PhpUnhandledExceptionCallback);
#endif

    return TRUE;
}

BOOLEAN PhInitializeNamespacePolicy(
    VOID
    )
{
    HANDLE mutantHandle;
    WCHAR objectName[PH_INT64_STR_LEN_1];
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectNameUs;
    PH_FORMAT format[2];

    PhInitFormatS(&format[0], L"PhMutant_");
    PhInitFormatU(&format[1], HandleToUlong(NtCurrentProcessId()));

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        objectName,
        sizeof(objectName),
        NULL
        ))
    {
        return FALSE;
    }

    RtlInitUnicodeString(&objectNameUs, objectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectNameUs,
        OBJ_CASE_INSENSITIVE,
        PhGetNamespaceHandle(),
        NULL
        );

    if (NT_SUCCESS(NtCreateMutant(
        &mutantHandle,
        MUTANT_QUERY_STATE,
        &objectAttributes,
        TRUE
        )))
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhInitializeMitigationPolicy(
    VOID
    )
{
#ifndef DEBUG
#define DEFAULT_MITIGATION_POLICY_FLAGS \
    (PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON | \
     PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON)

    static PH_STRINGREF nompCommandlinePart = PH_STRINGREF_INIT(L" -nomp");
    static PH_STRINGREF rasCommandlinePart = PH_STRINGREF_INIT(L" -ras");
    BOOLEAN success = TRUE;
    PH_STRINGREF commandlineSr;
    PPH_STRING commandline = NULL;
    PS_SYSTEM_DLL_INIT_BLOCK (*LdrSystemDllInitBlock_I) = NULL;
    STARTUPINFOEX startupInfo = { sizeof(STARTUPINFOEX) };
    SIZE_T attributeListLength;

    if (WindowsVersion < WINDOWS_10_RS3)
        return TRUE;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandlineSr);

    // NOTE: The SCM has a bug where calling CreateProcess with PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY to restart the service with mitigations
    // causes the SCM to spew EVENT_SERVICE_DIFFERENT_PID_CONNECTED in the system eventlog and terminate the service. (dmex)
    // WARN: This bug makes it impossible to start services with mitigation polices when the service doesn't have an IFEO key...
    if (PhFindStringInStringRef(&commandlineSr, &rasCommandlinePart, FALSE) != -1)
        return TRUE;
    if (PhEndsWithStringRef(&commandlineSr, &nompCommandlinePart, FALSE))
        return TRUE;

    if (!(LdrSystemDllInitBlock_I = PhGetDllProcedureAddress(L"ntdll.dll", "LdrSystemDllInitBlock", 0)))
        goto CleanupExit;

    if (!RTL_CONTAINS_FIELD(LdrSystemDllInitBlock_I, LdrSystemDllInitBlock_I->Size, MitigationOptionsMap))
        goto CleanupExit;

    if ((LdrSystemDllInitBlock_I->MitigationOptionsMap.Map[0] & DEFAULT_MITIGATION_POLICY_FLAGS) == DEFAULT_MITIGATION_POLICY_FLAGS)
        goto CleanupExit;

    if (!InitializeProcThreadAttributeList(NULL, 1, 0, &attributeListLength) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto CleanupExit;

    startupInfo.lpAttributeList = PhAllocate(attributeListLength);

    if (!InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, &attributeListLength))
        goto CleanupExit;

    if (!UpdateProcThreadAttribute(startupInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY, &(ULONG64){ DEFAULT_MITIGATION_POLICY_FLAGS }, sizeof(ULONG64), NULL, NULL))
        goto CleanupExit;

    commandline = PhConcatStringRef2(&commandlineSr, &nompCommandlinePart);

    if (NT_SUCCESS(PhCreateProcessWin32Ex(
        NULL,
        PhGetString(commandline),
        NULL,
        NULL,
        &startupInfo.StartupInfo,
        PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB,
        NULL,
        NULL,
        NULL,
        NULL
        )))
    {
        success = FALSE;
    }

CleanupExit:

    if (commandline)
        PhDereferenceObject(commandline);

    if (startupInfo.lpAttributeList)
    {
        DeleteProcThreadAttributeList(startupInfo.lpAttributeList);
        PhFree(startupInfo.lpAttributeList);
    }

    return success;
#else
    return TRUE;
#endif
}

NTSTATUS PhpReadSignature(
    _In_ PWSTR FileName,
    _Out_ PUCHAR *Signature,
    _Out_ PULONG SignatureSize
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PUCHAR signature;
    ULONG bufferSize;
    IO_STATUS_BLOCK iosb;

    if (!NT_SUCCESS(status = PhCreateFileWin32(&fileHandle, FileName, FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
    {
        return status;
    }

    bufferSize = 1024;
    signature = PhAllocate(bufferSize);

    status = NtReadFile(fileHandle, NULL, NULL, NULL, &iosb, signature, bufferSize, NULL, NULL);
    NtClose(fileHandle);

    if (NT_SUCCESS(status))
    {
        *Signature = signature;
        *SignatureSize = (ULONG)iosb.Information;
        return status;
    }
    else
    {
        PhFree(signature);
        return status;
    }
}

VOID PhpShowKphError(
    _In_ PWSTR Message, 
    _In_opt_ NTSTATUS Status
    )
{
    if (Status == STATUS_NO_SUCH_FILE)
    {
        PhShowError2(
            NULL,
            Message,
            L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
            );
    }
    else
    {
        PPH_STRING errorMessage;
        PPH_STRING statusMessage;

        if (errorMessage = PhGetStatusMessage(Status, 0))
        {
            statusMessage = PhConcatStrings(
                3,
                errorMessage->Buffer,
                L"\r\n\r\n",
                L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
                );
            PhShowError2(NULL, Message, statusMessage->Buffer);
            PhDereferenceObject(statusMessage);
            PhDereferenceObject(errorMessage);
        }
        else
        {
            PhShowError2(
                NULL, 
                Message, 
                L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
                );
        }
    }
}

VOID PhInitializeKph(
    VOID
    )
{
    NTSTATUS status;
    ULONG latestBuildNumber;
    PPH_STRING applicationDirectory;
    PPH_STRING kprocesshackerFileName;
    PPH_STRING processhackerSigFileName;
    KPH_PARAMETERS parameters;

    latestBuildNumber = PhGetIntegerSetting(L"KphBuildNumber");

    if (latestBuildNumber == 0)
    {
        PhSetIntegerSetting(L"KphBuildNumber", PhOsVersion.dwBuildNumber);
    }
    else
    {
        if (latestBuildNumber != PhOsVersion.dwBuildNumber)
        {
            // Reset KPH after a Windows build update. (dmex)
            if (NT_SUCCESS(KphResetParameters(KPH_DEVICE_SHORT_NAME)))
            {
                PhSetIntegerSetting(L"KphBuildNumber", PhOsVersion.dwBuildNumber);
            }
        }
    }

    if (!(applicationDirectory = PhGetApplicationDirectory()))
        return;

    kprocesshackerFileName = PhConcatStringRefZ(&applicationDirectory->sr, L"kprocesshacker.sys");
    processhackerSigFileName = PhConcatStringRefZ(&applicationDirectory->sr, L"ProcessHacker.sig");
    PhDereferenceObject(applicationDirectory);

    if (!PhDoesFileExistsWin32(kprocesshackerFileName->Buffer))
    {
        //if (PhGetIntegerSetting(L"EnableKphWarnings") && !PhStartupParameters.PhSvc)
        //    PhpShowKphError(L"The Process Hacker kernel driver 'kprocesshacker.sys' was not found in the application directory.", STATUS_NO_SUCH_FILE);
        return;
    }

    parameters.SecurityLevel = KphSecuritySignatureAndPrivilegeCheck;
    parameters.CreateDynamicConfiguration = TRUE;

    if (NT_SUCCESS(status = KphConnect2Ex(
        KPH_DEVICE_SHORT_NAME,
        kprocesshackerFileName->Buffer,
        &parameters
        )))
    {
        PUCHAR signature;
        ULONG signatureSize;

        status = PhpReadSignature(
            processhackerSigFileName->Buffer, 
            &signature, 
            &signatureSize
            );

        if (NT_SUCCESS(status))
        {
            status = KphVerifyClient(signature, signatureSize);

            if (!NT_SUCCESS(status))
            {
                if (PhGetIntegerSetting(L"EnableKphWarnings") && !PhStartupParameters.PhSvc)
                    PhpShowKphError(L"Unable to verify the kernel driver signature.", status);
            }

            PhFree(signature);
        }
        else
        {
            if (PhGetIntegerSetting(L"EnableKphWarnings") && !PhStartupParameters.PhSvc)
                PhpShowKphError(L"Unable to load the kernel driver signature.", status);
        }
    }
    else
    {
        if (PhGetIntegerSetting(L"EnableKphWarnings") && PhGetOwnTokenAttributes().Elevated && !PhStartupParameters.PhSvc)
            PhpShowKphError(L"Unable to load the kernel driver.", status);
    }

    PhDereferenceObject(kprocesshackerFileName);
    PhDereferenceObject(processhackerSigFileName);
}

BOOLEAN PhInitializeAppSystem(
    VOID
    )
{
    if (!PhProcessProviderInitialization())
        return FALSE;
    if (!PhServiceProviderInitialization())
        return FALSE;
    if (!PhNetworkProviderInitialization())
        return FALSE;

    PhSetHandleClientIdFunction(PhGetClientIdName);

    return TRUE;
}

VOID PhpInitializeSettings(
    VOID
    )
{
    NTSTATUS status;

    if (!PhStartupParameters.NoSettings)
    {
        static PH_STRINGREF settingsPath = PH_STRINGREF_INIT(L"%APPDATA%\\Process Hacker\\settings.xml");
        static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
        PPH_STRING settingsFileName;

        // There are three possible locations for the settings file:
        // 1. The file name given in the command line.
        // 2. A file named ProcessHacker.exe.settings.xml in the program directory. (This changes
        //    based on the executable file name.)
        // 3. The default location.

        // 1. File specified in command line
        if (PhStartupParameters.SettingsFileName)
        {
            // Get an absolute path now.
            PhSettingsFileName = PhGetFullPath(PhStartupParameters.SettingsFileName->Buffer, NULL);
        }

        // 2. File in program directory
        if (!PhSettingsFileName)
        {
            PPH_STRING applicationFileName;

            if (applicationFileName = PhGetApplicationFileName())
            {
                settingsFileName = PhConcatStringRef2(&applicationFileName->sr, &settingsSuffix);

                if (PhDoesFileExistsWin32(settingsFileName->Buffer))
                {
                    PhSettingsFileName = settingsFileName;
                }
                else
                {
                    PhDereferenceObject(settingsFileName);
                }

                PhDereferenceObject(applicationFileName);
            }
        }

        // 3. Default location
        if (!PhSettingsFileName)
        {
            PhSettingsFileName = PhExpandEnvironmentStrings(&settingsPath);
        }

        if (PhSettingsFileName)
        {
            status = PhLoadSettings(PhSettingsFileName->Buffer);

            // If we didn't find the file, it will be created. Otherwise,
            // there was probably a parsing error and we don't want to
            // change anything.
            if (status == STATUS_FILE_CORRUPT_ERROR)
            {
                if (PhShowMessage2(
                    NULL,
                    TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                    TD_WARNING_ICON,
                    L"Process Hacker's settings file is corrupt. Do you want to reset it?",
                    L"If you select No, the settings system will not function properly."
                    ) == IDYES)
                {
                    HANDLE fileHandle;
                    IO_STATUS_BLOCK isb;
                    CHAR data[] = "<settings></settings>";

                    // This used to delete the file. But it's better to keep the file there
                    // and overwrite it with some valid XML, especially with case (2) above.
                    if (NT_SUCCESS(PhCreateFileWin32(
                        &fileHandle,
                        PhSettingsFileName->Buffer,
                        FILE_GENERIC_WRITE,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_OVERWRITE,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                        )))
                    {
                        NtWriteFile(fileHandle, NULL, NULL, NULL, &isb, data, sizeof(data) - 1, NULL, NULL);
                        NtClose(fileHandle);
                    }
                }
                else
                {
                    // Pretend we don't have a settings store so bad things
                    // don't happen.
                    PhDereferenceObject(PhSettingsFileName);
                    PhSettingsFileName = NULL;
                }
            }
        }
    }

    // Apply basic global settings.
    PhPluginsEnabled = !!PhGetIntegerSetting(L"EnablePlugins");
    PhMaxSizeUnit = PhGetIntegerSetting(L"MaxSizeUnit");

    if (PhGetIntegerSetting(L"SampleCountAutomatic"))
    {
        ULONG sampleCount;

        sampleCount = (GetSystemMetrics(SM_CXVIRTUALSCREEN) + 1) / 2;

        if (sampleCount > 2048)
            sampleCount = 2048;

        PhSetIntegerSetting(L"SampleCount", sampleCount);
    }
}

#define PH_ARG_SETTINGS 1
#define PH_ARG_NOSETTINGS 2
#define PH_ARG_SHOWVISIBLE 3
#define PH_ARG_SHOWHIDDEN 4
#define PH_ARG_COMMANDMODE 5
#define PH_ARG_COMMANDTYPE 6
#define PH_ARG_COMMANDOBJECT 7
#define PH_ARG_COMMANDACTION 8
#define PH_ARG_COMMANDVALUE 9
#define PH_ARG_RUNASSERVICEMODE 10
#define PH_ARG_NOKPH 11
#define PH_ARG_INSTALLKPH 12
#define PH_ARG_UNINSTALLKPH 13
#define PH_ARG_DEBUG 14
#define PH_ARG_HWND 15
#define PH_ARG_POINT 16
#define PH_ARG_SHOWOPTIONS 17
#define PH_ARG_PHSVC 18
#define PH_ARG_NOPLUGINS 19
#define PH_ARG_NEWINSTANCE 20
#define PH_ARG_ELEVATE 21
#define PH_ARG_SILENT 22
#define PH_ARG_HELP 23
#define PH_ARG_SELECTPID 24
#define PH_ARG_PRIORITY 25
#define PH_ARG_PLUGIN 26
#define PH_ARG_SELECTTAB 27
#define PH_ARG_SYSINFO 28

BOOLEAN NTAPI PhpCommandLineOptionCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    ULONG64 integer;

    if (Option)
    {
        switch (Option->Id)
        {
        case PH_ARG_SETTINGS:
            PhSwapReference(&PhStartupParameters.SettingsFileName, Value);
            break;
        case PH_ARG_NOSETTINGS:
            PhStartupParameters.NoSettings = TRUE;
            break;
        case PH_ARG_SHOWVISIBLE:
            PhStartupParameters.ShowVisible = TRUE;
            break;
        case PH_ARG_SHOWHIDDEN:
            PhStartupParameters.ShowHidden = TRUE;
            break;
        case PH_ARG_COMMANDMODE:
            PhStartupParameters.CommandMode = TRUE;
            break;
        case PH_ARG_COMMANDTYPE:
            PhSwapReference(&PhStartupParameters.CommandType, Value);
            break;
        case PH_ARG_COMMANDOBJECT:
            PhSwapReference(&PhStartupParameters.CommandObject, Value);
            break;
        case PH_ARG_COMMANDACTION:
            PhSwapReference(&PhStartupParameters.CommandAction, Value);
            break;
        case PH_ARG_COMMANDVALUE:
            PhSwapReference(&PhStartupParameters.CommandValue, Value);
            break;
        case PH_ARG_RUNASSERVICEMODE:
            PhSwapReference(&PhStartupParameters.RunAsServiceMode, Value);
            break;
        case PH_ARG_NOKPH:
            PhStartupParameters.NoKph = TRUE;
            break;
        case PH_ARG_INSTALLKPH:
            PhStartupParameters.InstallKph = TRUE;
            break;
        case PH_ARG_UNINSTALLKPH:
            PhStartupParameters.UninstallKph = TRUE;
            break;
        case PH_ARG_DEBUG:
            PhStartupParameters.Debug = TRUE;
            break;
        case PH_ARG_HWND:
            if (Value && PhStringToInteger64(&Value->sr, 16, &integer))
                PhStartupParameters.WindowHandle = (HWND)(ULONG_PTR)integer;
            break;
        case PH_ARG_POINT:
            {
                PH_STRINGREF xString;
                PH_STRINGREF yString;

                if (Value && PhSplitStringRefAtChar(&Value->sr, ',', &xString, &yString))
                {
                    LONG64 x;
                    LONG64 y;

                    if (PhStringToInteger64(&xString, 10, &x) && PhStringToInteger64(&yString, 10, &y))
                    {
                        PhStartupParameters.Point.x = (LONG)x;
                        PhStartupParameters.Point.y = (LONG)y;
                    }
                }
            }
            break;
        case PH_ARG_SHOWOPTIONS:
            PhStartupParameters.ShowOptions = TRUE;
            break;
        case PH_ARG_PHSVC:
            PhStartupParameters.PhSvc = TRUE;
            break;
        case PH_ARG_NOPLUGINS:
            PhStartupParameters.NoPlugins = TRUE;
            break;
        case PH_ARG_NEWINSTANCE:
            PhStartupParameters.NewInstance = TRUE;
            break;
        case PH_ARG_ELEVATE:
            PhStartupParameters.Elevate = TRUE;
            break;
        case PH_ARG_SILENT:
            PhStartupParameters.Silent = TRUE;
            break;
        case PH_ARG_HELP:
            PhStartupParameters.Help = TRUE;
            break;
        case PH_ARG_SELECTPID:
            if (Value && PhStringToInteger64(&Value->sr, 0, &integer))
                PhStartupParameters.SelectPid = (ULONG)integer;
            break;
        case PH_ARG_PRIORITY:
            if (Value && PhEqualString2(Value, L"r", TRUE))
                PhStartupParameters.PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;
            else if (Value && PhEqualString2(Value, L"h", TRUE))
                PhStartupParameters.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
            else if (Value && PhEqualString2(Value, L"n", TRUE))
                PhStartupParameters.PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
            else if (Value && PhEqualString2(Value, L"l", TRUE))
                PhStartupParameters.PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
            break;
        case PH_ARG_PLUGIN:
            if (!PhStartupParameters.PluginParameters)
                PhStartupParameters.PluginParameters = PhCreateList(3);
            if (Value)
                PhAddItemList(PhStartupParameters.PluginParameters, PhReferenceObject(Value));
            break;
        case PH_ARG_SELECTTAB:
            PhSwapReference(&PhStartupParameters.SelectTab, Value);
            break;
        case PH_ARG_SYSINFO:
            PhSwapReference(&PhStartupParameters.SysInfo, Value ? Value : PhReferenceEmptyString());
            break;
        }
    }
    else
    {
        PPH_STRING upperValue = NULL;

        if (Value)
            upperValue = PhUpperString(Value);

        if (upperValue)
        {
            if (PhFindStringInString(upperValue, 0, L"TASKMGR.EXE") != -1)
            {
                // User probably has Process Hacker replacing Task Manager. Force
                // the main window to start visible.
                PhStartupParameters.ShowVisible = TRUE;
            }

            PhDereferenceObject(upperValue);
        }
    }

    return TRUE;
}

VOID PhpProcessStartupParameters(
    VOID
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_ARG_SETTINGS, L"settings", MandatoryArgumentType },
        { PH_ARG_NOSETTINGS, L"nosettings", NoArgumentType },
        { PH_ARG_SHOWVISIBLE, L"v", NoArgumentType },
        { PH_ARG_SHOWHIDDEN, L"hide", NoArgumentType },
        { PH_ARG_COMMANDMODE, L"c", NoArgumentType },
        { PH_ARG_COMMANDTYPE, L"ctype", MandatoryArgumentType },
        { PH_ARG_COMMANDOBJECT, L"cobject", MandatoryArgumentType },
        { PH_ARG_COMMANDACTION, L"caction", MandatoryArgumentType },
        { PH_ARG_COMMANDVALUE, L"cvalue", MandatoryArgumentType },
        { PH_ARG_RUNASSERVICEMODE, L"ras", MandatoryArgumentType },
        { PH_ARG_NOKPH, L"nokph", NoArgumentType },
        { PH_ARG_INSTALLKPH, L"installkph", NoArgumentType },
        { PH_ARG_UNINSTALLKPH, L"uninstallkph", NoArgumentType },
        { PH_ARG_DEBUG, L"debug", NoArgumentType },
        { PH_ARG_HWND, L"hwnd", MandatoryArgumentType },
        { PH_ARG_POINT, L"point", MandatoryArgumentType },
        { PH_ARG_SHOWOPTIONS, L"showoptions", NoArgumentType },
        { PH_ARG_PHSVC, L"phsvc", NoArgumentType },
        { PH_ARG_NOPLUGINS, L"noplugins", NoArgumentType },
        { PH_ARG_NEWINSTANCE, L"newinstance", NoArgumentType },
        { PH_ARG_ELEVATE, L"elevate", NoArgumentType },
        { PH_ARG_SILENT, L"s", NoArgumentType },
        { PH_ARG_HELP, L"help", NoArgumentType },
        { PH_ARG_SELECTPID, L"selectpid", MandatoryArgumentType },
        { PH_ARG_PRIORITY, L"priority", MandatoryArgumentType },
        { PH_ARG_PLUGIN, L"plugin", MandatoryArgumentType },
        { PH_ARG_SELECTTAB, L"selecttab", MandatoryArgumentType },
        { PH_ARG_SYSINFO, L"sysinfo", OptionalArgumentType }
    };
    PH_STRINGREF commandLine;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandLine);

    memset(&PhStartupParameters, 0, sizeof(PH_STARTUP_PARAMETERS));

    if (!PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS | PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PhpCommandLineOptionCallback,
        NULL
        ) || PhStartupParameters.Help)
    {
        PhShowInformation(
            NULL,
            L"Command line options:\n\n"
            L"-c\n"
            L"-ctype command-type\n"
            L"-cobject command-object\n"
            L"-caction command-action\n"
            L"-cvalue command-value\n"
            L"-debug\n"
            L"-elevate\n"
            L"-help\n"
            L"-hide\n"
            L"-installkph\n"
            L"-newinstance\n"
            L"-nokph\n"
            L"-noplugins\n"
            L"-nosettings\n"
            L"-plugin pluginname:value\n"
            L"-priority r|h|n|l\n"
            L"-s\n"
            L"-selectpid pid-to-select\n"
            L"-selecttab name-of-tab-to-select\n"
            L"-settings filename\n"
            L"-sysinfo [section-name]\n"
            L"-uninstallkph\n"
            L"-v\n"
            );

        if (PhStartupParameters.Help)
            RtlExitUserProcess(STATUS_SUCCESS);
    }

    if (PhStartupParameters.InstallKph)
    {
        NTSTATUS status;
        PPH_STRING applicationDirectory;
        PPH_STRING kprocesshackerFileName;
        KPH_PARAMETERS parameters;

        if (!(applicationDirectory = PhGetApplicationDirectory()))
            return;

        kprocesshackerFileName = PhConcatStringRefZ(&applicationDirectory->sr, L"\\kprocesshacker.sys");
        PhDereferenceObject(applicationDirectory);

        parameters.SecurityLevel = KphSecuritySignatureCheck;
        parameters.CreateDynamicConfiguration = TRUE;

        status = KphInstallEx(KPH_DEVICE_SHORT_NAME, kprocesshackerFileName->Buffer, &parameters);

        if (!NT_SUCCESS(status) && !PhStartupParameters.Silent)
            PhShowStatus(NULL, L"Unable to install KProcessHacker", status, 0);

        RtlExitUserProcess(status);
    }

    if (PhStartupParameters.UninstallKph)
    {
        NTSTATUS status;

        status = KphUninstall(KPH_DEVICE_SHORT_NAME);

        if (!NT_SUCCESS(status) && !PhStartupParameters.Silent)
            PhShowStatus(NULL, L"Unable to uninstall KProcessHacker", status, 0);

        RtlExitUserProcess(status);
    }

    if (PhStartupParameters.Elevate && !PhGetOwnTokenAttributes().Elevated)
    {
        PhShellProcessHacker(
            NULL,
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_FORCE_SETTINGS,
            0,
            NULL
            );
        RtlExitUserProcess(STATUS_SUCCESS);
    }

    if (PhStartupParameters.Debug)
    {
        // The symbol provider won't work if this is chosen.
        PhShowDebugConsole();
    }
}

VOID PhpEnablePrivileges(
    VOID
    )
{
    HANDLE tokenHandle;

    if (NT_SUCCESS(PhOpenProcessToken(
        NtCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES,
        &tokenHandle
        )))
    {
        CHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * 9];
        PTOKEN_PRIVILEGES privileges;
        ULONG i;

        privileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
        privileges->PrivilegeCount = 9;

        for (i = 0; i < privileges->PrivilegeCount; i++)
        {
            privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
            privileges->Privileges[i].Luid.HighPart = 0;
        }

        privileges->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
        privileges->Privileges[1].Luid.LowPart = SE_INC_BASE_PRIORITY_PRIVILEGE;
        privileges->Privileges[2].Luid.LowPart = SE_INC_WORKING_SET_PRIVILEGE;
        privileges->Privileges[3].Luid.LowPart = SE_LOAD_DRIVER_PRIVILEGE;
        privileges->Privileges[4].Luid.LowPart = SE_PROF_SINGLE_PROCESS_PRIVILEGE;
        privileges->Privileges[5].Luid.LowPart = SE_BACKUP_PRIVILEGE;
        privileges->Privileges[6].Luid.LowPart = SE_RESTORE_PRIVILEGE;
        privileges->Privileges[7].Luid.LowPart = SE_SHUTDOWN_PRIVILEGE;
        privileges->Privileges[8].Luid.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;

        NtAdjustPrivilegesToken(
            tokenHandle,
            FALSE,
            privileges,
            0,
            NULL,
            NULL
            );

        NtClose(tokenHandle);
    }
}
