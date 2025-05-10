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

#include <phapp.h>
#include <colorbox.h>
#include <hexedit.h>
#include <hndlinfo.h>
#include <objbase.h>

#include <extmgri.h>
#include <mainwnd.h>
#include <netprv.h>
#include <phconsole.h>
#include <phsettings.h>
#include <phsvc.h>
#include <procprv.h>
#include <devprv.h>
#include <notifico.h>

#include <ksisup.h>
#include <settings.h>
#include <srvprv.h>

LONG PhMainMessageLoop(
    VOID
    );

VOID PhActivatePreviousInstance(
    VOID
    );

VOID PhInitializeCommonControls(
    VOID
    );

VOID PhInitializeSuperclassControls( // delayhook.c
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

VOID PhEnableTerminationPolicy(
    _In_ BOOLEAN Enabled
    );

BOOLEAN PhInitializeDirectoryPolicy(
    VOID
    );

BOOLEAN PhInitializeExceptionPolicy(
    VOID
    );

BOOLEAN PhInitializeMitigationPolicy(
    VOID
    );

BOOLEAN PhInitializeComPolicy(
    VOID
    );

BOOLEAN PhInitializeTimerPolicy(
    VOID
    );

BOOLEAN PhPluginsEnabled = FALSE;
BOOLEAN PhPortableEnabled = FALSE;
PPH_STRING PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS PhStartupParameters = { .UpdateChannel = PhInvalidChannel };

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;
PH_PROVIDER_THREAD PhTertiaryProviderThread;

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

    if (!NT_SUCCESS(PhInitializePhLib(L"System Informer", Instance)))
        return 1;
    if (!PhInitializeDirectoryPolicy())
        return 1;
    if (!PhInitializeExceptionPolicy())
        return 1;
    if (!PhInitializeComPolicy())
        return 1;

    PhpProcessStartupParameters();
    PhpEnablePrivileges();

    if (PhStartupParameters.RunAsServiceMode)
    {
        PhExitApplication(PhRunAsServiceStart(PhStartupParameters.RunAsServiceMode));
    }

    PhGuiSupportInitialization();
    PhpInitializeSettings();

    if (PhGetIntegerSetting(L"AllowOnlyOneInstance") &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.PhSvc &&
        !PhStartupParameters.KphStartupHigh &&
        !PhStartupParameters.KphStartupMax)
    {
        PhActivatePreviousInstance();
    }

    if (PhGetIntegerSetting(L"EnableStartAsAdmin") &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.PhSvc)
    {
        if (PhGetOwnTokenAttributes().Elevated)
        {
            if (PhGetIntegerSetting(L"EnableStartAsAdminAlwaysOnTop"))
            {
                if (NT_SUCCESS(PhRunAsAdminTaskUIAccess()))
                {
                    PhActivatePreviousInstance();
                    PhExitApplication(STATUS_SUCCESS);
                }
            }
        }
        else
        {
            if (SUCCEEDED(PhRunAsAdminTask(&SI_RUNAS_ADMIN_TASK_NAME)))
            {
                PhActivatePreviousInstance();
                PhExitApplication(STATUS_SUCCESS);
            }
        }
    }

    PhInitializeSuperclassControls();

    if (PhEnableKsiSupport &&
        !PhStartupParameters.ShowOptions)
    {
        PhInitializeKsi();
    }

#ifdef DEBUG
    dbg.ClientId = NtCurrentTeb()->ClientId;
    dbg.StartAddress = wWinMain;
    dbg.Parameter = NULL;
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    PhTlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    PhInitializeAutoPool(&BaseAutoPool);

    PhInitializeCommonControls();
    PhTreeNewInitialization();
    PhGraphControlInitialization();
    PhHexEditInitialization();
    PhColorBoxInitialization();

    PhInitializeAppSystem();
    PhInitializeCallbacks();
    PhEmInitialization();

    if (PhStartupParameters.ShowOptions)
    {
        PhShowOptionsDialog(PhStartupParameters.WindowHandle);
        PhExitApplication(STATUS_SUCCESS);
    }

    if (PhPluginsEnabled && !PhStartupParameters.NoPlugins)
    {
        PhLoadPlugins();
    }

    // N.B. Must be called after loading plugins since we set Microsoft signed only.
    if (!PhInitializeMitigationPolicy())
        return 1;

    if (PhStartupParameters.PhSvc)
    {
        MSG message;

        // Turn the feedback cursor off.
        PostMessage(NULL, WM_NULL, 0, 0);
        GetMessage(&message, NULL, 0, 0);

        PhExitApplication(PhSvcMain(NULL, NULL));
    }

#ifndef DEBUG
    if (PhIsExecutingInWow64())
    {
        PhShowWarning2(
            NULL,
            L"Warning.",
            L"%s",
            L"You are attempting to run the 32-bit version of System Informer on 64-bit Windows. "
            L"Most features will not work correctly.\n\n"
            L"Please run the 64-bit version of System Informer instead."
            );
        PhExitApplication(STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT);
    }
#endif

    // Set the default timer resolution.
    {
        if (WindowsVersion > WINDOWS_11)
        {
            PhSetProcessPowerThrottlingState(
                NtCurrentProcess(),
                POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION,
                0  // Disable synthetic timer resolution.
                );
        }
    }

    // Set the default priority.
    {
        UCHAR priorityClass = PROCESS_PRIORITY_CLASS_HIGH;

        if (PhStartupParameters.PriorityClass != 0)
            priorityClass = (UCHAR)PhStartupParameters.PriorityClass;

        PhSetProcessPriorityClass(NtCurrentProcess(), priorityClass);
    }

    if (PhEnableKsiSupport)
    {
        PhShowKsiStatus();
    }

    if (!PhMainWndInitialization(CmdShow))
    {
        PhShowStatus(NULL, L"Unable to create the window.", 0, ERROR_OUTOFMEMORY);
        return 1;
    }

    PhEnableTerminationPolicy(TRUE);

    PhDrainAutoPool(&BaseAutoPool);

    result = PhMainMessageLoop();

    PhEnableTerminationPolicy(FALSE);

    if (PhEnableKsiSupport)
    {
        PhCleanupKsi();
    }

    PhExitApplication(result);
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

    if (indexOfFilter != ULONG_MAX)
        PhRemoveItemList(FilterList, indexOfFilter);

    PhFree(FilterEntry);
}

typedef struct _PHP_PREVIOUS_MAIN_WINDOW_CONTEXT
{
    HANDLE ProcessId;
    PPH_STRING WindowName;
} PHP_PREVIOUS_MAIN_WINDOW_CONTEXT, *PPHP_PREVIOUS_MAIN_WINDOW_CONTEXT;

static BOOL CALLBACK PhPreviousInstanceWindowEnumProc(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
    PPHP_PREVIOUS_MAIN_WINDOW_CONTEXT context = (PPHP_PREVIOUS_MAIN_WINDOW_CONTEXT)Context;
    CLIENT_ID clientId;

    if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
        return TRUE;

    if (clientId.UniqueProcess == context->ProcessId)
    {
        WCHAR className[256];

        if (GetClassName(WindowHandle, className, RTL_NUMBER_OF(className)))
        {
            if (PhEqualStringZ(className, PhGetString(context->WindowName), FALSE))
            {
                ULONG_PTR result = 0;

                SendMessageTimeout(
                    WindowHandle,
                    WM_PH_ACTIVATE,
                    PhStartupParameters.SelectPid,
                    0,
                    SMTO_ABORTIFHUNG | SMTO_BLOCK,
                    5000,
                    &result
                    );

                if (result == PH_ACTIVATE_REPLY)
                {
                    SetForegroundWindow(WindowHandle);

                    PhExitApplication(STATUS_SUCCESS);
                }
            }
        }
    }

    return TRUE;
}

static VOID PhForegroundPreviousInstance(
    _In_ HANDLE ProcessId
    )
{
    HANDLE processHandle = NULL;
    HANDLE tokenHandle = NULL;
    PH_TOKEN_USER tokenUser;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId)))
        goto CleanupExit;
    if (!NT_SUCCESS(PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle)))
        goto CleanupExit;
    if (!NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
        goto CleanupExit;

    if (PhEqualSid(tokenUser.User.Sid, PhGetOwnTokenAttributes().TokenSid))
    {
        PHP_PREVIOUS_MAIN_WINDOW_CONTEXT context;
        ULONG attempts = 50;

        memset(&context, 0, sizeof(PHP_PREVIOUS_MAIN_WINDOW_CONTEXT));
        context.ProcessId = ProcessId;
        context.WindowName = PhGetStringSetting(L"MainWindowClassName");

        do
        {
            if (PhIsNullOrEmptyString(context.WindowName))
                break;

            PhEnumWindows(PhPreviousInstanceWindowEnumProc, &context);

            PhDelayExecution(100);
        } while (--attempts != 0);

        PhClearReference(&context.WindowName);
    }

CleanupExit:
    if (tokenHandle)
    {
        NtClose(tokenHandle);
    }

    if (processHandle)
    {
        NtClose(processHandle);
    }
}

VOID PhActivatePreviousInstance(
    VOID
    )
{
    HANDLE fileHandle;
    PPH_STRING applicationFileName;

    if (applicationFileName = PhGetApplicationFileName())
    {
        if (NT_SUCCESS(PhOpenFile(
            &fileHandle,
            &applicationFileName->sr,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            NULL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            )))
        {
            PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;

            if (NT_SUCCESS(PhGetProcessIdsUsingFile(
                fileHandle,
                &processIds
                )))
            {
                for (ULONG i = 0; i < processIds->NumberOfProcessIdsInList; i++)
                {
                    HANDLE processId = processIds->ProcessIdList[i];
                    PPH_STRING fileName;

                    if (processId == NtCurrentProcessId())
                        continue;

                    if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(processId, &fileName)))
                    {
                        if (PhEqualString(applicationFileName, fileName, TRUE))
                        {
                            PhForegroundPreviousInstance(processId);
                        }

                        PhDereferenceObject(fileName);
                    }
                }

                PhFree(processIds);
            }

            NtClose(fileHandle);
        }

        PhDereferenceObject(applicationFileName);
    }
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

BOOLEAN PhInitializeDirectoryPolicy(
    VOID
    )
{
    PPH_STRING applicationDirectory;
    UNICODE_STRING applicationDirectoryUs;
    PH_STRINGREF currentDirectory;

    if (!(applicationDirectory = PhGetApplicationDirectoryWin32()))
        return FALSE;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath, &currentDirectory);

    if (PhEqualStringRef(&applicationDirectory->sr, &currentDirectory, TRUE))
    {
        PhDereferenceObject(applicationDirectory);
        return TRUE;
    }

    if (!PhStringRefToUnicodeString(&applicationDirectory->sr, &applicationDirectoryUs))
    {
        PhDereferenceObject(applicationDirectory);
        return FALSE;
    }

    if (!NT_SUCCESS(RtlSetCurrentDirectory_U(&applicationDirectoryUs)))
    {
        PhDereferenceObject(applicationDirectory);
        return FALSE;
    }

    PhDereferenceObject(applicationDirectory);
    return TRUE;
}

#pragma region Error Reporting
#include <symprv.h>
#include <winsta.h>

typedef enum _PH_TRIAGE_DUMP_TYPE
{
    PhTriageDumpTypeMinimal,
    PhTriageDumpTypeNormal,
    PhTriageDumpTypeFull,
} PH_TRIAGE_DUMP_TYPE;

VOID PhpCreateUnhandledExceptionCrashDump(
    _In_ PEXCEPTION_POINTERS ExceptionInfo,
    _In_ PH_TRIAGE_DUMP_TYPE DumpType
    )
{
    HANDLE fileHandle;
    PPH_STRING directory;
    PPH_STRING fileName;
    WCHAR alphastring[16] = L"";

    PhGenerateRandomAlphaString(alphastring, RTL_NUMBER_OF(alphastring));
    directory = PhExpandEnvironmentStringsZ(L"\\??\\%USERPROFILE%\\Desktop\\");
    fileName = PhConcatStrings(5, PhGetString(directory), L"SystemInformer", L"_DumpFile_", alphastring, L".dmp");
    PhCreateDirectoryFullPath(&fileName->sr);

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        ULONG dumpType = PhTriageDumpTypeMinimal;

        exceptionInfo.ThreadId = HandleToUlong(NtCurrentThreadId());
        exceptionInfo.ExceptionPointers = ExceptionInfo;
        exceptionInfo.ClientPointers = TRUE;

        switch (DumpType)
        {
        case PhTriageDumpTypeMinimal:
            dumpType =
                MiniDumpWithDataSegs |
                MiniDumpWithUnloadedModules |
                MiniDumpWithProcessThreadData |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory;
            break;
        case PhTriageDumpTypeNormal:
            dumpType =
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpScanMemory |
                MiniDumpWithUnloadedModules |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithTokenInformation;
            break;
        case PhTriageDumpTypeFull:
            dumpType =
                MiniDumpWithDataSegs |
                MiniDumpWithFullMemory |
                MiniDumpWithHandleData |
                MiniDumpWithUnloadedModules |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithTokenInformation |
                MiniDumpWithAvxXStateContext;
            break;
        }

        PhWriteMiniDumpProcess(
            NtCurrentProcess(),
            NtCurrentProcessId(),
            fileHandle,
            dumpType,
            &exceptionInfo,
            NULL,
            NULL
            );

        NtClose(fileHandle);
    }

    PhDereferenceObject(fileName);
    PhDereferenceObject(directory);
}

ULONG CALLBACK PhpUnhandledExceptionCallback(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    )
{
    PPH_STRING errorMessage;
    PPH_STRING message;
    INT result;

    // Let the debugger handle the exception. (dmex)
    if (PhIsDebuggerPresent())
    {
        // Remove this return to debug the exception callback. (dmex)
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (PhIsInteractiveUserSession())
    {
        TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
        TASKDIALOG_BUTTON buttons[6] =
        {
            { 101, L"Full\nA complete dump of the process, rarely needed most of the time." },
            { 102, L"Normal\nFor most purposes, this dump file is the most useful." },
            { 103, L"Minimal\nA very limited dump with limited data." },
            { 104, L"Restart\nRestart the application." }, // and hope it doesn't crash again.";
            { 105, L"Ignore" },  // \nTry ignore the exception and continue.";
            { 106, L"Exit" }, // \nTerminate the program.";
        };

        if (NT_NTWIN32(ExceptionInfo->ExceptionRecord->ExceptionCode))
            errorMessage = PhGetStatusMessage(0, PhNtStatusToDosError(ExceptionInfo->ExceptionRecord->ExceptionCode));
        else
            errorMessage = PhGetStatusMessage(ExceptionInfo->ExceptionRecord->ExceptionCode, 0);

        message = PhFormatString(
            L"0x%08X (%s)",
            ExceptionInfo->ExceptionRecord->ExceptionCode,
            PhGetStringOrEmpty(errorMessage)
            );

        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS | TDF_EXPAND_FOOTER_AREA;
        config.pszWindowTitle = PhApplicationName;
        config.pszMainIcon = TD_ERROR_ICON;
        config.pszMainInstruction = L"System Informer has crashed :(";
        config.cButtons = RTL_NUMBER_OF(buttons);
        config.pButtons = buttons;
        config.nDefaultButton = 106;
        config.cxWidth = 250;
        config.pszContent = PhGetString(message);
#ifdef DEBUG
        config.pszExpandedInformation = PhGetString(PhGetStacktraceAsString());
#endif

        if (PhShowTaskDialog(&config, &result, NULL, NULL))
        {
            switch (result)
            {
            case 101:
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo, PhTriageDumpTypeFull);
                break;
            case 102:
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo, PhTriageDumpTypeNormal);
                break;
            case 103:
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo, PhTriageDumpTypeMinimal);
                break;
            case 104:
                {
                    PhShellProcessHacker(
                        NULL,
                        NULL,
                        SW_SHOW,
                        PH_SHELL_EXECUTE_DEFAULT,
                        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                        0,
                        NULL
                        );
                }
                break;
            case 105:
                {
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
                break;
            }
        }
        else
        {
            if (PhShowMessage(
                NULL,
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
                L"System Informer has crashed :(\r\n\r\n%s",
                L"Do you want to create a minidump on the Desktop?"
                ) == IDYES)
            {
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo, PhTriageDumpTypeMinimal);
            }
        }
    }
    else
    {
        ULONG response;
        PPH_STRING title;

        if (NT_NTWIN32(ExceptionInfo->ExceptionRecord->ExceptionCode))
            errorMessage = PhGetStatusMessage(0, PhNtStatusToDosError(ExceptionInfo->ExceptionRecord->ExceptionCode));
        else
            errorMessage = PhGetStatusMessage(ExceptionInfo->ExceptionRecord->ExceptionCode, 0);

        title = PhCreateString(L"System Informer has crashed :(");
#ifdef DEBUG
        message = PhFormatString(
            L"%s\r\n0x%08X (%s)\r\n%s",
            PhGetStringOrEmpty(title),
            ExceptionInfo->ExceptionRecord->ExceptionCode,
            PhGetStringOrEmpty(errorMessage),
            PhGetString(PhGetStacktraceAsString())
            );
#else
        message = PhFormatString(
            L"%s\r\n0x%08X (%s)\r\n%s",
            PhGetStringOrEmpty(title),
            ExceptionInfo->ExceptionRecord->ExceptionCode,
            PhGetStringOrEmpty(errorMessage)
            );
#endif
        if (WinStationSendMessageW(
            SERVERNAME_CURRENT,
            USER_SHARED_DATA->ActiveConsoleId, // RtlGetActiveConsoleId
            title->Buffer,
            (ULONG)title->Length,
            message->Buffer,
            (ULONG)message->Length,
            MB_OKCANCEL | MB_ICONERROR,
            30,
            &response,
            FALSE
            ))
        {
#ifdef DEBUG
            if (response == IDOK)
            {
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo, PhTriageDumpTypeFull);
            }
#endif
        }
    }

    PhExitApplication(ExceptionInfo->ExceptionRecord->ExceptionCode);

    return EXCEPTION_EXECUTE_HANDLER;
}
#pragma endregion

BOOLEAN PhInitializeExceptionPolicy(
    VOID
    )
{
#if PHNT_MINIMAL_ERRORMODE
    ULONG errorMode;

    if (NT_SUCCESS(PhGetProcessErrorMode(NtCurrentProcess(), &errorMode)))
    {
        ClearFlag(errorMode, SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        PhSetProcessErrorMode(NtCurrentProcess(), errorMode);
    }
#else
    PhSetProcessErrorMode(NtCurrentProcess(), 0);
#endif
    SetUnhandledExceptionFilter(PhpUnhandledExceptionCallback);

    return TRUE;
}

BOOLEAN PhInitializeMitigationPolicy(
    VOID
    )
{
#if defined(PH_BUILD_API)
    if (WindowsVersion >= WINDOWS_10)
    {
        PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

        //policyInfo.Policy = ProcessDynamicCodePolicy;
        //policyInfo.DynamicCodePolicy.Flags = 0;
        //policyInfo.DynamicCodePolicy.ProhibitDynamicCode = TRUE;
        //NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

        //policyInfo.Policy = ProcessExtensionPointDisablePolicy;
        //policyInfo.ExtensionPointDisablePolicy.Flags = 0;
        //policyInfo.ExtensionPointDisablePolicy.DisableExtensionPoints = TRUE;
        //NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

        policyInfo.Policy = ProcessSignaturePolicy;
        policyInfo.SignaturePolicy.Flags = 0;
        policyInfo.SignaturePolicy.MicrosoftSignedOnly = TRUE;
        NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

        //policyInfo.Policy = ProcessRedirectionTrustPolicy;
        //policyInfo.RedirectionTrustPolicy.Flags = 0;
        //policyInfo.RedirectionTrustPolicy.EnforceRedirectionTrust = TRUE;
        //NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    }
#endif
    return TRUE;
}

BOOLEAN PhInitializeComPolicy(
    VOID
    )
{
#ifdef PH_COM_SVC
    #include <cguid.h>
    IGlobalOptions* globalOptions;
    UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x300];
    PSECURITY_DESCRIPTOR securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
    PSID administratorsSid = PhSeAdministratorsSid();
    ULONG securityDescriptorAllocationLength;
    PACL dacl;

    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return TRUE; // Continue without COM support. (dmex)

    if (SUCCEEDED(PhGetClassObject(L"combase.dll", &CLSID_GlobalOptions, &IID_IGlobalOptions, &globalOptions)))
    {
        #define COMGLB_PROPERTIES_EXPLICIT_WINSTA_DESKTOP COMGLB_PROPERTIES_RESERVED1
        #define COMGLB_PROCESS_MITIGATION_POLICY_BLOB COMGLB_PROPERTIES_RESERVED2
        #define COMGLB_ENABLE_AGILE_OOP_PROXIES COMGLB_RESERVED1
        #define COMGLB_ENABLE_WAKE_ON_RPC_SUPPRESSION COMGLB_RESERVED2
        #define COMGLB_ADD_RESTRICTEDCODE_SID_TO_COM_CALLPERMISSIONS COMGLB_RESERVED3
        #define HKLM_ONLY_CLASSIC_COM_CATALOG COMGLB_RESERVED5

        IGlobalOptions_Set(globalOptions, COMGLB_EXCEPTION_HANDLING, COMGLB_EXCEPTION_DONOT_HANDLE_ANY);
        IGlobalOptions_Set(globalOptions, COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN | COMGLB_ENABLE_AGILE_OOP_PROXIES | HKLM_ONLY_CLASSIC_COM_CATALOG);
        IGlobalOptions_Release(globalOptions);
    }

    securityDescriptorAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid(&PhSeAuthenticatedUserSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid(&PhSeLocalSystemSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid(&administratorsSid);

    dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
    PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    PhCreateAcl(dacl, securityDescriptorAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    PhAddAccessAllowedAce(dacl, ACL_REVISION, FILE_READ_DATA | FILE_WRITE_DATA, &PhSeAuthenticatedUserSid);
    PhAddAccessAllowedAce(dacl, ACL_REVISION, FILE_READ_DATA | FILE_WRITE_DATA, &PhSeLocalSystemSid);
    PhAddAccessAllowedAce(dacl, ACL_REVISION, FILE_READ_DATA | FILE_WRITE_DATA, administratorsSid);
    PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);
    PhSetGroupSecurityDescriptor(securityDescriptor, administratorsSid, FALSE);
    PhSetOwnerSecurityDescriptor(securityDescriptor, administratorsSid, FALSE);

    if (!SUCCEEDED(CoInitializeSecurity(
        securityDescriptor,
        UINT_MAX,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IDENTIFY,
        NULL,
        EOAC_NONE,
        NULL
        )))
    {
        NOTHING;
    }

#ifdef DEBUG
    assert(securityDescriptorAllocationLength < sizeof(securityDescriptorBuffer));
    assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
#endif

    return TRUE;
#else
    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        NOTHING;

    return TRUE;
#endif
}

VOID PhEnableTerminationPolicy(
    _In_ BOOLEAN Enabled
    )
{
    if (PhGetOwnTokenAttributes().Elevated && PhGetIntegerSetting(L"EnableBreakOnTermination"))
    {
        NTSTATUS status;

        status = PhSetProcessBreakOnTermination(
            NtCurrentProcess(),
            Enabled
            );

        if (Enabled)
            SetFlag(NtCurrentPeb()->NtGlobalFlag, FLG_ENABLE_SYSTEM_CRIT_BREAKS);
        else
            ClearFlag(NtCurrentPeb()->NtGlobalFlag, FLG_ENABLE_SYSTEM_CRIT_BREAKS);

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(NULL, L"Unable to configure termination policy.", status, 0);
        }
    }
}

BOOLEAN PhInitializeTimerPolicy(
    VOID
    )
{
    static BOOL timerSuppression = FALSE;

    SetUserObjectInformation(NtCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &timerSuppression, sizeof(BOOL));

    return TRUE;
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
    PhSettingsInitialization();
    PhAddDefaultSettings();

    if (!PhStartupParameters.NoSettings)
    {
        static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
        PPH_STRING settingsFileName;

        // There are three possible locations for the settings file:
        // 1. The file name given in the command line.
        // 2. A file named SystemInformer.exe.settings.xml in the program directory. (This changes
        //    based on the executable file name.)
        // 3. The default location.

        // 1. File specified in command line
        if (PhStartupParameters.SettingsFileName)
        {
            // Get an absolute path now.
            PhGetFullPath(PhStartupParameters.SettingsFileName->Buffer, &PhSettingsFileName, NULL);
        }

        // 2. File in program directory
        if (PhIsNullOrEmptyString(PhSettingsFileName))
        {
            PPH_STRING applicationFileName;

            if (applicationFileName = PhGetApplicationFileName())
            {
                settingsFileName = PhConcatStringRef2(&applicationFileName->sr, &settingsSuffix);

                if (PhDoesFileExist(&settingsFileName->sr))
                {
                    PhSettingsFileName = settingsFileName;
                    PhPortableEnabled = TRUE;
                }
                else
                {
                    PhDereferenceObject(settingsFileName);
                }

                PhDereferenceObject(applicationFileName);
            }
        }

        // 3. Default location
        if (PhIsNullOrEmptyString(PhSettingsFileName))
        {
            PhSettingsFileName = PhGetKnownLocationZ(PH_FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.xml", TRUE);
        }

        if (!PhIsNullOrEmptyString(PhSettingsFileName))
        {
            NTSTATUS status;

            status = PhLoadSettings(&PhSettingsFileName->sr);

            // If we didn't find the file, it will be created. Otherwise,
            // there was probably a parsing error and we don't want to
            // change anything.
            if (status == STATUS_FILE_CORRUPT_ERROR)
            {
                if (PhShowMessage2(
                    NULL,
                    TD_YES_BUTTON | TD_NO_BUTTON,
                    TD_WARNING_ICON,
                    L"System Informer's settings file is corrupt. Do you want to reset it?",
                    L"If you select No, the settings system will not function properly."
                    ) == IDYES)
                {
                    HANDLE fileHandle;
                    IO_STATUS_BLOCK isb;
                    CHAR data[] = "<settings></settings>";

                    // This used to delete the file. But it's better to keep the file there
                    // and overwrite it with some valid XML, especially with case (2) above.
                    if (NT_SUCCESS(PhCreateFile(
                        &fileHandle,
                        &PhSettingsFileName->sr,
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

    PhUpdateCachedSettings();

    // Apply basic global settings.
    PhPluginsEnabled = !!PhGetIntegerSetting(L"EnablePlugins");
    PhMaxSizeUnit = PhGetIntegerSetting(L"MaxSizeUnit");
    PhMaxPrecisionUnit = (USHORT)PhGetIntegerSetting(L"MaxPrecisionUnit");
    PhMaxPrecisionLimit = 1.0f;
    for (ULONG i = 0; i < PhMaxPrecisionUnit; i++)
        PhMaxPrecisionLimit /= 10;
    PhEnableWindowText = !!PhGetIntegerSetting(L"EnableWindowText");
    PhEnableThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport");
    PhThemeWindowForegroundColor = PhGetIntegerSetting(L"ThemeWindowForegroundColor");
    PhThemeWindowBackgroundColor = PhGetIntegerSetting(L"ThemeWindowBackgroundColor");
    PhThemeWindowBackground2Color = PhGetIntegerSetting(L"ThemeWindowBackground2Color");
    PhThemeWindowHighlightColor = PhGetIntegerSetting(L"ThemeWindowHighlightColor");
    PhThemeWindowHighlight2Color = PhGetIntegerSetting(L"ThemeWindowHighlight2Color");
    PhThemeWindowTextColor = PhGetIntegerSetting(L"ThemeWindowTextColor");
    PhEnableThemeAcrylicSupport = WindowsVersion >= WINDOWS_11 && !!PhGetIntegerSetting(L"EnableThemeAcrylicSupport");
    PhEnableThemeAcrylicWindowSupport = WindowsVersion >= WINDOWS_11 && !!PhGetIntegerSetting(L"EnableThemeAcrylicWindowSupport");
    PhEnableThemeNativeButtons = !!PhGetIntegerSetting(L"EnableThemeNativeButtons");
    PhEnableThemeListviewBorder = !!PhGetIntegerSetting(L"TreeListBorderEnable");
    PhEnableDeferredLayout = !!PhGetIntegerSetting(L"EnableDeferredLayout");
    PhEnableServiceNonPoll = !!PhGetIntegerSetting(L"EnableServiceNonPoll");
    PhEnableServiceNonPollNotify = !!PhGetIntegerSetting(L"EnableServiceNonPollNotify");
    PhServiceNonPollFlushInterval = PhGetIntegerSetting(L"NonPollFlushInterval");
    PhEnableKsiSupport = !!PhGetIntegerSetting(L"KsiEnable") && !PhStartupParameters.NoKph && !PhIsExecutingInWow64();
    PhEnableKsiWarnings = !!PhGetIntegerSetting(L"KsiEnableWarnings");
    PhFontQuality = PhGetFontQualitySetting(PhGetIntegerSetting(L"FontQuality"));

    if (PhGetIntegerSetting(L"SampleCountAutomatic"))
    {
        ULONG sampleCount;

        sampleCount = (PhGetSystemMetrics(SM_CXVIRTUALSCREEN, 0) + 1) / 2;

        if (sampleCount > 4096)
            sampleCount = 4096;

        PhSetIntegerSetting(L"SampleCount", sampleCount);
    }

    if (PhStartupParameters.UpdateChannel != PhInvalidChannel)
    {
        PhSetIntegerSetting(L"ReleaseChannel", PhStartupParameters.UpdateChannel);
    }

    if (PhStartupParameters.ShowHidden && !PhNfIconsEnabled())
    {
        // HACK(jxy-s) The default used to be that system tray icons where enabled, this keeps the
        // old behavior for automation workflows. If the user specified "-hide" then they want to
        // start the program hidden to the system tray and not show any main window. If there are no
        // system tray icons enabled then we need to enable them so the behavior is consistent.
        PhSetStringSetting(L"IconSettings", L"2|1");
    }
}

typedef enum _PH_COMMAND_ARG
{
    PH_ARG_NONE,
    PH_ARG_SETTINGS,
    PH_ARG_NOSETTINGS,
    PH_ARG_SHOWVISIBLE,
    PH_ARG_SHOWHIDDEN,
    PH_ARG_RUNASSERVICEMODE,
    PH_ARG_NOKPH,
    PH_ARG_DEBUG,
    PH_ARG_HWND,
    PH_ARG_POINT,
    PH_ARG_SHOWOPTIONS,
    PH_ARG_PHSVC,
    PH_ARG_NOPLUGINS,
    PH_ARG_NEWINSTANCE,
    PH_ARG_ELEVATE,
    PH_ARG_SILENT,
    PH_ARG_HELP,
    PH_ARG_SELECTPID,
    PH_ARG_PRIORITY,
    PH_ARG_PLUGIN,
    PH_ARG_SELECTTAB,
    PH_ARG_SYSINFO,
    PH_ARG_KPHSTARTUPHIGH,
    PH_ARG_KPHSTARTUPMAX,
    PH_ARG_CHANNEL,
} PH_COMMAND_ARG;

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
        case PH_ARG_RUNASSERVICEMODE:
            PhSwapReference(&PhStartupParameters.RunAsServiceMode, Value);
            break;
        case PH_ARG_NOKPH:
            PhStartupParameters.NoKph = TRUE;
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

                if (Value && PhSplitStringRefAtChar(&Value->sr, L',', &xString, &yString))
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
        case PH_ARG_KPHSTARTUPHIGH:
            PhStartupParameters.KphStartupHigh = TRUE;
            break;
        case PH_ARG_KPHSTARTUPMAX:
            PhStartupParameters.KphStartupMax = TRUE;
            break;
        case PH_ARG_CHANNEL:
            {
                if (Value && PhEqualString2(Value, L"release", FALSE))
                    PhStartupParameters.UpdateChannel = PhReleaseChannel;
                else if (Value && PhEqualString2(Value, L"preview", FALSE))
                    PhStartupParameters.UpdateChannel = PhPreviewChannel;
                else if (Value && PhEqualString2(Value, L"canary", FALSE))
                    PhStartupParameters.UpdateChannel = PhCanaryChannel;
                else if (Value && PhEqualString2(Value, L"developer", FALSE))
                    PhStartupParameters.UpdateChannel = PhDeveloperChannel;
            }
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
            if (PhFindStringInString(upperValue, 0, L"TASKMGR.EXE") != SIZE_MAX)
            {
                // User probably has System Informer replacing Task Manager. Force
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
    PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_ARG_SETTINGS, L"settings", MandatoryArgumentType },
        { PH_ARG_NOSETTINGS, L"nosettings", NoArgumentType },
        { PH_ARG_SHOWVISIBLE, L"v", NoArgumentType },
        { PH_ARG_SHOWHIDDEN, L"hide", NoArgumentType },
        { PH_ARG_RUNASSERVICEMODE, L"ras", MandatoryArgumentType },
        { PH_ARG_NOKPH, L"nokph", NoArgumentType },
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
        { PH_ARG_SYSINFO, L"sysinfo", OptionalArgumentType },
        { PH_ARG_KPHSTARTUPHIGH, L"kh", NoArgumentType },
        { PH_ARG_KPHSTARTUPMAX, L"kx", NoArgumentType },
        { PH_ARG_CHANNEL, L"channel", MandatoryArgumentType },
    };
    PH_STRINGREF commandLine;

    PhGetProcessCommandLineStringRef(&commandLine);

    if (!PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS | PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PhpCommandLineOptionCallback,
        NULL
        ) || PhStartupParameters.Help)
    {
        PhShowInformation2(
            NULL,
            L"Command line options:",
            L"%s",
            L"-debug\n"
            L"-elevate\n"
            L"-help\n"
            L"-hide\n"
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
            L"-channel [channel-name]\n"
            L"-v"
            );

        PhExitApplication(STATUS_SUCCESS);
    }

    if (PhStartupParameters.Elevate && !PhGetOwnTokenAttributes().Elevated)
    {
        PhShellProcessHacker(
            NULL,
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            PH_SHELL_APP_PROPAGATE_PARAMETERS,
            0,
            NULL
            );
        PhExitApplication(STATUS_SUCCESS);
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
        const LUID_AND_ATTRIBUTES privileges[] =
        {
            { RtlConvertUlongToLuid(SE_DEBUG_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_INC_BASE_PRIORITY_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_INC_WORKING_SET_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_LOAD_DRIVER_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_PROF_SINGLE_PROCESS_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_BACKUP_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_RESTORE_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_SHUTDOWN_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_TAKE_OWNERSHIP_PRIVILEGE), SE_PRIVILEGE_ENABLED },
            { RtlConvertUlongToLuid(SE_SECURITY_PRIVILEGE), SE_PRIVILEGE_ENABLED },
        };
        UCHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(privileges)];
        PTOKEN_PRIVILEGES tokenPrivileges;

        tokenPrivileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
        tokenPrivileges->PrivilegeCount = RTL_NUMBER_OF(privileges);
        memcpy(tokenPrivileges->Privileges, privileges, sizeof(privileges));

        NtAdjustPrivilegesToken(
            tokenHandle,
            FALSE,
            tokenPrivileges,
            0,
            NULL,
            NULL
            );

        NtClose(tokenHandle);
    }
}
