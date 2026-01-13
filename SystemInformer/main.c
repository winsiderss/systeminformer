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
#include <mainwndp.h>
#include <netprv.h>
#include <phsettings.h>
#include <phsvc.h>
#include <procprv.h>
#include <devprv.h>
#include <notifico.h>

#include <ksisup.h>
#include <settings.h>
#include <srvprv.h>

#include <trace.h>

BOOLEAN PhPluginsEnabled = FALSE;
BOOLEAN PhPortableEnabled = FALSE;
PPH_STRING PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS PhStartupParameters = { .UpdateChannel = PhInvalidChannel };
PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;
PH_PROVIDER_THREAD PhTertiaryProviderThread;
RTL_ATOM PhTreeWindowAtom = RTL_ATOM_INVALID_ATOM;
RTL_ATOM PhGraphWindowAtom = RTL_ATOM_INVALID_ATOM;
RTL_ATOM PhHexEditWindowAtom = RTL_ATOM_INVALID_ATOM;
RTL_ATOM PhColorBoxWindowAtom = RTL_ATOM_INVALID_ATOM;
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

    if (!NT_SUCCESS(PhInitializePhLib(L"System Informer")))
        return 1;
    if (!NT_SUCCESS(PhInitializeDirectoryPolicy()))
        return 1;
    if (!NT_SUCCESS(PhInitializeExceptionPolicy()))
        return 1;
    if (!NT_SUCCESS(PhInitializeExecutionPolicy()))
        return 1;
    if (!NT_SUCCESS(PhInitializeNamespacePolicy()))
        return 1;
    if (!NT_SUCCESS(PhInitializeComPolicy()))
        return 1;

    PhpProcessStartupParameters();
    PhpEnablePrivileges();

    if (PhStartupParameters.RunAsServiceMode)
    {
        PhExitApplication(PhRunAsServiceStart(PhStartupParameters.RunAsServiceMode));
    }

    PhGuiSupportInitialization();

    PhInitializeAppSettings();

    if (PhStartupParameters.Debug)
    {
        PhShowDebugConsole();
    }

    PhInitializePreviousInstance();

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

    PhInitializeSuperclassControls();
    PhInitializeCommonControls();

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
    if (!NT_SUCCESS(PhInitializeMitigationPolicy()))
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
        if (WindowsVersion >= WINDOWS_11_24H2)
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
    return result;
}

/**
 * The main message loop for System Informer.
 *
 * Processes Windows messages for the main window, dialogs, and registered message loop filters.
 * Handles accelerator keys, dialog messages, and dispatches messages to the appropriate handlers.
 * Drains the auto pool after each message to manage memory efficiently.
 * \return The exit code of the application, typically the wParam of the WM_QUIT message.
 */
LONG PhMainMessageLoop(
    VOID
    )
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(NtCurrentImageBase(), MAKEINTRESOURCE(IDR_MAINWND_ACCEL));

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

/**
 * Registers a dialog window handle with the application's dialog list.
 *
 * This function adds the specified dialog window handle to the internal list of dialogs
 * managed by the application. This allows the main message loop to recognize and process
 * messages for the dialog, enabling features such as keyboard navigation and message routing.
 *
 * \param DialogWindowHandle The handle to the dialog window to register.
 * \remarks Registered dialogs are processed in the main message loop for dialog-specific messages.
 */
VOID PhRegisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    if (!DialogList)
        DialogList = PhCreateList(2);

    PhAddItemList(DialogList, (PVOID)DialogWindowHandle);
}

/**
 * Unregisters a dialog window handle from the application's dialog list.
 *
 * This function removes the specified dialog window handle from the internal list of dialogs
 * managed by the application. This ensures that the main message loop no longer processes
 * messages for the dialog, disabling features such as keyboard navigation and message routing
 * for the unregistered dialog.
 *
 * \param DialogWindowHandle The handle to the dialog window to unregister.
 * \remarks Unregistered dialogs are no longer processed in the main message loop for dialog-specific messages.
 */
VOID PhUnregisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    ULONG indexOfDialog;

    if (!DialogList)
        return;

    indexOfDialog = PhFindItemList(DialogList, (PVOID)DialogWindowHandle);

    if (indexOfDialog != ULONG_MAX)
        PhRemoveItemList(DialogList, indexOfDialog);
}

/**
 * Registers a message loop filter with the application's message loop.
 *
 * This function adds a custom filter to the internal list of message loop filters.
 * Each filter is a callback that can intercept and process Windows messages before
 * they are handled by the main window or dialogs. Filters are useful for implementing
 * global hotkeys, custom message handling, or preprocessing messages.
 *
 * \param Filter The filter callback function to register.
 * \param Context Optional context pointer passed to the filter callback.
 * \return A pointer to the filter entry, which can be used to unregister the filter later.
 * \remarks The filter will be called for each message in the main message loop.
 */
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

/**
 * Unregisters a message loop filter from the application's message loop.
 *
 * This function removes a previously registered message loop filter, ensuring
 * it no longer receives messages from the main message loop.
 *
 * \param FilterEntry The filter entry returned by PhRegisterMessageLoopFilter.
 * \remarks The filter entry is freed after removal.
 */
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

/**
 * Context structure for tracking a previous main window instance.
 *
 * Used during enumeration of windows to identify and interact with a previous
 * instance of the application, based on process ID and window class name.
 */
typedef struct _PHP_PREVIOUS_MAIN_WINDOW_CONTEXT
{
    HANDLE ProcessId;
    PPH_STRING WindowName;
} PHP_PREVIOUS_MAIN_WINDOW_CONTEXT, *PPHP_PREVIOUS_MAIN_WINDOW_CONTEXT;

/**
 * Callback function for enumerating windows to find a previous instance.
 *
 * Checks if the given window belongs to the specified process and matches the
 * expected window class name. If found, attempts to activate and bring the window
 * to the foreground.
 *
 * \param WindowHandle The handle to the window being enumerated.
 * \param Context Pointer to a PHP_PREVIOUS_MAIN_WINDOW_CONTEXT structure.
 * \return FALSE if the previous instance window was found and activated; TRUE to continue enumeration.
 */
_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PhPreviousInstanceWindowEnumProc(
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

                PhTrace(
                    "Found previous instance window: %ls (%p)",
                    className,
                    WindowHandle
                    );

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
                    if (!IsWindowVisible(WindowHandle))
                    {
                        ShowWindow(WindowHandle, SW_SHOW);
                    }

                    if (IsIconic(WindowHandle))
                    {
                        ShowWindow(WindowHandle, SW_RESTORE);
                    }

                    if (!SetForegroundWindow(WindowHandle))
                    {
                        PhBringWindowToTop(WindowHandle);
                    }

                    PhExitApplication(STATUS_SUCCESS);
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

/**
 * Brings a previous instance of the application to the foreground.
 *
 * This function attempts to locate the main window of a previous instance of the
 * application (by process ID and window class name), and brings it to the foreground.
 * It performs several checks to ensure the process is in the same session and owned
 * by the same user. The search is retried multiple times to handle race conditions.
 * \param ProcessId The process ID of the previous instance.
 */
static VOID PhForegroundPreviousInstance(
    _In_ HANDLE ProcessId
    )
{
    PHP_PREVIOUS_MAIN_WINDOW_CONTEXT context;
    HANDLE processHandle = NULL;
    HANDLE tokenHandle = NULL;
    PH_TOKEN_USER tokenUser;
    ULONG sessionId = 0;
    ULONG attempts = 0;

    PhTraceFuncEnter("Foreground previous instance: %lu", HandleToUlong(ProcessId));

    memset(&context, 0, sizeof(PHP_PREVIOUS_MAIN_WINDOW_CONTEXT));
    context.ProcessId = ProcessId;
    context.WindowName = PhGetStringSetting(SETTING_MAIN_WINDOW_CLASS_NAME);

    if (PhIsNullOrEmptyString(context.WindowName))
        goto CleanupExit;
    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId)))
        goto CleanupExit;
    if (!NT_SUCCESS(PhGetProcessSessionId(processHandle, &sessionId)))
        goto CleanupExit;
    if (NtCurrentPeb()->SessionId != sessionId)
        goto CleanupExit;
    if (!NT_SUCCESS(PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle)))
        goto CleanupExit;
    if (!NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
        goto CleanupExit;
    if (!PhEqualSid(tokenUser.User.Sid, PhGetOwnTokenAttributes().TokenSid))
        goto CleanupExit;

    // Try to locate the window a few times because some users reported that it might not yet have been created. (dmex)
    do
    {
        PhEnumWindowsEx(NULL, PhPreviousInstanceWindowEnumProc, &context);
        PhDelayExecution(500);
    } while (++attempts < 10);

CleanupExit:

    if (tokenHandle)
    {
        NtClose(tokenHandle);
    }

    if (processHandle)
    {
        NtClose(processHandle);
    }

    PhClearReference(&context.WindowName);

    PhTraceFuncExit(
        "Foreground previous instance: %lu (%lu attempts)",
        HandleToUlong(ProcessId),
        attempts
        );
}

/**
 * Initializes previous instance detection and activation logic.
 *
 * This function enforces single-instance behavior and handles elevation scenarios.
 * If only one instance is allowed, it attempts to activate a previous instance.
 * If elevation is required, it runs the application as administrator and activates
 * the previous instance if necessary.
 */
VOID PhInitializePreviousInstance(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_ALLOW_ONLY_ONE_INSTANCE) &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.PhSvc &&
        !PhStartupParameters.KphStartupHigh &&
        !PhStartupParameters.KphStartupMax)
    {
        PhActivatePreviousInstance();
    }

    if (PhGetIntegerSetting(SETTING_ENABLE_START_AS_ADMIN) &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.PhSvc)
    {
        if (PhGetOwnTokenAttributes().Elevated)
        {
            if (PhGetIntegerSetting(SETTING_ENABLE_START_AS_ADMIN_ALWAYS_ON_TOP))
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
}

/**
 * Callback for enumerating namespace objects to find previous instances.
 *
 * This function is called for each object in the namespace directory. It looks for
 * mutant objects and, if found, checks if they belong to a different process.
 * If so, it attempts to bring the previous instance to the foreground.
 *
 * \param RootDirectory The root directory handle.
 * \param Name The name of the object.
 * \param TypeName The type name of the object.
 * \param Context Optional context pointer.
 * \return STATUS_NO_MORE_ENTRIES if a previous instance
 * was found and activated; otherwise STATUS_SUCCESS.
 */
_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
NTSTATUS NTAPI PhpPreviousInstancesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    MUTANT_OWNER_INFORMATION objectInfo;
    HANDLE objectHandle;

    if (!PhStartsWithStringRef2(Name, L"SiMutant_", FALSE))
        return STATUS_NAME_TOO_LONG;

    if (NT_SUCCESS(PhOpenMutant(
        &objectHandle,
        MUTANT_QUERY_STATE,
        RootDirectory,
        Name
        )))
    {
        if (NT_SUCCESS(PhGetMutantOwnerInformation(
            objectHandle,
            &objectInfo
            )))
        {
            if (objectInfo.ClientId.UniqueProcess != NtCurrentProcessId())
            {
                PhForegroundPreviousInstance(objectInfo.ClientId.UniqueProcess);

                NtClose(objectHandle);
                return STATUS_NO_MORE_ENTRIES;
            }
        }

        NtClose(objectHandle);
    }

    return STATUS_SUCCESS;
}

/**
 * Activates a previous instance of the application if one exists.
 *
 * This function enumerates namespace objects to find a mutant object representing
 * a previous instance. If found, it brings the previous instance's window to the
 * foreground and exits the current process.
 */
VOID PhActivatePreviousInstance(
    VOID
    )
{
    NTSTATUS status;
    //HANDLE fileHandle;
    //PPH_STRING applicationFileName;

    PhTraceFuncEnter("Activate previous instance");

    status = PhEnumDirectoryObjects(PhGetNamespaceHandle(), PhpPreviousInstancesCallback, NULL);

    //if (applicationFileName = PhGetApplicationFileName())
    //{
    //    if (NT_SUCCESS(status = PhOpenFile(
    //        &fileHandle,
    //        &applicationFileName->sr,
    //        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
    //        NULL,
    //        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    //        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
    //        NULL
    //        )))
    //
    //    {
    //        PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;
    //
    //        if (NT_SUCCESS(status = PhGetProcessIdsUsingFile(
    //            fileHandle,
    //            &processIds
    //            )))
    //
    //        {
    //            for (ULONG i = 0; i < processIds->NumberOfProcessIdsInList; i++)
    //            {
    //                HANDLE processId = processIds->ProcessIdList[i];
    //                PPH_STRING fileName;
    //
    //                if (processId == NtCurrentProcessId())
    //                    continue;
    //
    //                if (NT_SUCCESS(status = PhGetProcessImageFileNameByProcessId(processId, &fileName)))
    //                {
    //                    if (PhEqualString(applicationFileName, fileName, TRUE))
    //                    {
    //                        PhForegroundPreviousInstance(processId);
    //                    }
    //
    //                    PhDereferenceObject(fileName);
    //                }
    //            }
    //
    //            PhFree(processIds);
    //        }
    //
    //        NtClose(fileHandle);
    //    }
    //
    //    PhDereferenceObject(applicationFileName);
    //}

    PhTraceFuncExit("Activate previous instance: %!STATUS!", status);
}

/**
 * Initializes common controls and custom controls used by the application.
 *
 * This function registers standard Windows common controls and initializes
 * custom controls such as tree views, graphs, hex editors, and color boxes.
 * It must be called before creating windows that use these controls.
 */
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

    PhTreeWindowAtom = PhTreeNewInitialization();
    PhGraphWindowAtom = PhGraphControlInitialization();
    PhHexEditWindowAtom = PhHexEditInitialization();
    PhColorBoxWindowAtom = PhColorBoxInitialization();
}

/**
 * Ensures the current working directory is set to the application's directory.
 *
 * This function checks if the process's current directory matches the application's
 * directory. If not, it sets the current directory to the application's directory.
 */
NTSTATUS PhInitializeDirectoryPolicy(
    VOID
    )
{
    PPH_STRING applicationDirectory;
    UNICODE_STRING applicationDirectoryUs;
    PH_STRINGREF currentDirectory;

    if (!(applicationDirectory = PhGetApplicationDirectoryWin32()))
        return STATUS_UNSUCCESSFUL;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath, &currentDirectory);

    if (PhEqualStringRef(&applicationDirectory->sr, &currentDirectory, TRUE))
    {
        PhDereferenceObject(applicationDirectory);
        return STATUS_SUCCESS;
    }

    if (!PhStringRefToUnicodeString(&applicationDirectory->sr, &applicationDirectoryUs))
    {
        PhDereferenceObject(applicationDirectory);
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(RtlSetCurrentDirectory_U(&applicationDirectoryUs)))
    {
        PhDereferenceObject(applicationDirectory);
        return STATUS_UNSUCCESSFUL;
    }

    PhDereferenceObject(applicationDirectory);
    return STATUS_SUCCESS;
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

/**
 * Creates a crash dump file for an unhandled exception.
 *
 * \param ExceptionInfo Pointer to the exception information.
 * \param DumpType The type of dump to create (minimal, normal, or full).
 */
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

static LPTOP_LEVEL_EXCEPTION_FILTER PhpPreviousUnhandledExceptionFilter = NULL;

/**
 * Unhandled exception filter callback for System Informer.
 *
 * This function is called when an unhandled exception occurs. It presents the user
 * with options to create a crash dump, restart, ignore, or exit. It also generates
 * a crash dump file on the desktop if requested.
 * \param ExceptionInfo Pointer to the exception information.
 * \return An exception disposition value.
 */
LONG CALLBACK PhpUnhandledExceptionCallback(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    )
{
    PPH_STRING errorMessage;
    PPH_STRING message;
    LONG result;

    // Let the debugger handle the exception. (dmex)
    if (PhIsDebuggerPresent())
    {
        // Remove this return to debug the exception callback. (dmex)
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (NT_SUCCESS(PhIsInteractiveUserSession()))
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

    return PhpPreviousUnhandledExceptionFilter(ExceptionInfo);
}
#pragma endregion

/**
 * Initializes the exception policy for the current process.
 *
 * This function configures the process error mode to suppress critical error dialogs
 * and disables the Windows error reporting dialog for unhandled exceptions. It also
 * installs a custom unhandled exception filter to handle application crashes, generate
 * crash dumps, and present user-friendly dialogs or messages.
 */
NTSTATUS PhInitializeExceptionPolicy(
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
    PhpPreviousUnhandledExceptionFilter = SetUnhandledExceptionFilter(PhpUnhandledExceptionCallback);

    return STATUS_SUCCESS;
}

/**
 * Initializes the namespace policy for the current process.
 *
 * This function creates a named mutant object in the process's namespace to enforce
 * single-instance behavior and activate previous instances of System Informer.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhInitializeNamespacePolicy(
    VOID
    )
{
    HANDLE mutantHandle;
    SIZE_T returnLength;
    WCHAR formatBuffer[PH_INT64_STR_LEN_1];
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    PH_STRINGREF objectNameSr;
    PH_FORMAT format[2];

    PhInitFormatS(&format[0], L"SiMutant_");
    PhInitFormatU(&format[1], HandleToUlong(NtCurrentProcessId()));

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        formatBuffer,
        sizeof(formatBuffer),
        &returnLength
        ))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
    objectNameSr.Buffer = formatBuffer;

    if (!PhStringRefToUnicodeString(&objectNameSr, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        PhGetNamespaceHandle(),
        NULL
        );

    NtCreateMutant(
        &mutantHandle,
        MUTANT_QUERY_STATE,
        &objectAttributes,
        TRUE
        );

    return STATUS_SUCCESS;
}

/**
 * Initializes the default COM options for the current process.
 *
 * If PH_COM_SVC is defined, it configures global COM options and sets a custom security descriptor
 * that allows access to authenticated users, local system, and administrators. Otherwise, it simply
 * initializes COM with apartment threading and disables OLE1 DDE.
 */
NTSTATUS PhInitializeComPolicy(
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
    assert(RtlValidSecurityDescriptor(securityDescriptor));
    assert(securityDescriptorAllocationLength < sizeof(securityDescriptorBuffer));
    assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
#endif

    return TRUE;
#else
    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        NOTHING;

    return STATUS_SUCCESS;
#endif
}

/**
 * Initializes the execution policy for System Informer.
 *
 * This function checks if the Shift key is held down during startup. If so, it attempts
 * to launch Task Manager (`taskmgr.exe`) instead of System Informer.
 * This provides a quick way for users to access Task Manager if needed, for example,
 * if System Informer is set as the default Task Manager replacement and the user
 * wants to access the original Task Manager without changing settings.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhInitializeExecutionPolicy(
    VOID
    )
{
    // Note: GetAsyncKeyState queries the global keyboard bitmask without kernel transitions, blocking,
    // handles, events or messages etc... The bitmask is also independent of any message loop or window.
    // We can call it extremely early but only the high bit (0x8000) of the key state is valid without a message loop.

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        if (NT_SUCCESS(PhShellExecuteEx(NULL, L"taskmgr.exe", NULL, NULL, SW_SHOW, 0, 0, NULL)))
        {
            PhExitApplication(STATUS_SUCCESS);
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Initializes the mitigation policies for the current process.
 *
 * This function configures process-level mitigations to prevent crashes by third party software. *
 * The function uses the Native API to set the mitigation policy. If the operating system does not
 * support the required mitigation policies, the function performs no action and returns success.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhInitializeMitigationPolicy(
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

        policyInfo.Policy = ProcessExtensionPointDisablePolicy;
        policyInfo.ExtensionPointDisablePolicy.Flags = 0;
        policyInfo.ExtensionPointDisablePolicy.DisableExtensionPoints = TRUE;
        NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

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
    return STATUS_SUCCESS;
}

/**
 * Initializes a temporary desktop policy for process isolation or UI testing.
 *
 * This function creates a new random desktop, switches the current thread and input
 * to that desktop, and launches a new instance of System Informer on it. After the
 * child process exits, it restores the original desktop and cleans up resources.
 *
 * \remarks The function is typically used for scenarios where process isolation or UI testing
 * is required on a separate desktop, such as security testing or debugging.
 */
VOID PhInitializeDesktopPolicy(
    VOID
    )
{
    NTSTATUS status;
    HDESK desktopHandle;
    WCHAR alphastring[16] = L"";

    PhGenerateRandomAlphaString(alphastring, RTL_NUMBER_OF(alphastring));

    if (desktopHandle = CreateDesktop(
        alphastring,
        NULL,
        NULL,
        0,
        DESKTOP_CREATEWINDOW,
        NULL
        ))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    if (NT_SUCCESS(status))
    {
        HANDLE processHandle;
        HDESK processDesktop;

        processDesktop = GetThreadDesktop(HandleToUlong(NtCurrentThreadId()));

        SetThreadDesktop(desktopHandle);
        SwitchDesktop(desktopHandle);

        if (NT_SUCCESS(PhShellProcessHacker(
            NULL,
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_DEFAULT,
            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
            0,
            &processHandle
            )))
        {
            PhWaitForSingleObject(processHandle, INFINITE);
            NtClose(processHandle);
        }

        SwitchDesktop(processDesktop);
        SetThreadDesktop(processDesktop);

        CloseDesktop(desktopHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to initialize desktop policy.", status, 0);
    }

    PhExitApplication(status);
}

/**
 * Enables or disables the process break-on-termination policy.
 *
 * This function configures the process to trigger a system critical break if it is terminated,
 * based on the specified flag. When enabled, the process is protected from termination by
 * non-privileged users, and the system will generate a critical error if the process is killed.
 * This is typically used to prevent accidental or unauthorized termination of critical processes.
 * The function only applies the policy if the current process is running with elevated privileges
 * and the "Enable Break On Termination" setting is enabled.
 * \param Enabled TRUE to enable the break-on-termination policy; FALSE to disable it.
 */
VOID PhEnableTerminationPolicy(
    _In_ BOOLEAN Enabled
    )
{
    if (PhGetOwnTokenAttributes().Elevated && PhGetIntegerSetting(SETTING_ENABLE_BREAK_ON_TERMINATION))
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

/**
 * Initializes the timer policy for the system.
 *
 * This function sets up and configures the timer policy that will be used
 * throughout the System Informer application for timing operations and measurements.
 */
NTSTATUS PhInitializeTimerPolicy(
    VOID
    )
{
    static BOOL timerSuppression = FALSE;

    SetUserObjectInformation(NtCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &timerSuppression, sizeof(BOOL));

    return STATUS_SUCCESS;
}

/**
 * Initializes the application system.
 * 
 * This function performs the necessary initialization of the System Informer
 * application system, setting up core components and resources required for
 * the application to function properly.
 */
NTSTATUS PhInitializeAppSystem(
    VOID
    )
{
    if (!PhProcessProviderInitialization())
        return STATUS_UNSUCCESSFUL;
    if (!PhServiceProviderInitialization())
        return STATUS_UNSUCCESSFUL;
    if (!PhNetworkProviderInitialization())
        return STATUS_UNSUCCESSFUL;

    PhSetHandleClientIdFunction(PhGetClientIdName);

    return STATUS_SUCCESS;
}

/**
 * Initializes the application settings.
 *
 * This function sets up and initializes all application-level settings,
 * including user preferences, configuration values, and default parameters
 * required for the System Informer application to function properly.
 */
VOID PhInitializeAppSettings(
    VOID
    )
{
    PhSettingsInitialization();
    PhAddDefaultSettings();

    if (!PhStartupParameters.NoSettings)
    {
        // There are three possible locations for the settings file:
        // 1. The file name given in the command line.
        // 2. A file named SystemInformer.exe.settings.json in the program directory. (This changes
        //    based on the executable file name.)
        // 3. The default location.

        // 1. File specified in command line
        if (PhStartupParameters.SettingsFileName)
        {
            if (PhDetermineDosPathNameType(&PhStartupParameters.SettingsFileName->sr) == RtlPathTypeRooted)
            {
                PhSetReference(&PhSettingsFileName, PhStartupParameters.SettingsFileName);
            }
            else
            {
                PPH_STRING settingsFileName;

                // Get an absolute path now.
                if (NT_SUCCESS(PhGetFullPath(PhGetString(PhStartupParameters.SettingsFileName), &settingsFileName, NULL)))
                {
                    PhMoveReference(&settingsFileName, PhDosPathNameToNtPathName(&settingsFileName->sr));

                    if (!PhIsNullOrEmptyString(settingsFileName))
                    {
                        PhMoveReference(&PhSettingsFileName, settingsFileName);
                    }
                }
            }
        }

        // 2. File in program directory
        if (PhIsNullOrEmptyString(PhSettingsFileName))
        {
            PPH_STRING settingsFileName;

            // Try .settings.json first
            if (settingsFileName = PhGetApplicationFileNameZ(L".settings.json"))
            {
                if (PhDoesFileExist(&settingsFileName->sr))
                {
                    PhMoveReference(&PhSettingsFileName, settingsFileName);
                    PhPortableEnabled = TRUE;
                }
                else
                {
                    PhDereferenceObject(settingsFileName);

                    // Try .settings.xml (legacy)
                    if (settingsFileName = PhGetApplicationFileNameZ(L".settings.xml"))
                    {
                        if (PhDoesFileExist(&settingsFileName->sr))
                        {
                            PPH_STRING jsonFileName = PhGetBaseNameChangeExtensionZ(&settingsFileName->sr, L".json");

                            // Convert XML to JSON
                            NTSTATUS convertStatus = PhConvertSettingsXmlToJson(
                                &settingsFileName->sr,
                                &jsonFileName->sr
                                );

                            if (NT_SUCCESS(convertStatus))
                            {
                                PhMoveReference(&PhSettingsFileName, jsonFileName);
                                PhPortableEnabled = TRUE;
                            }
                            else
                            {
                                PhDereferenceObject(jsonFileName);
                            }
                        }
                        else
                        {
                            PhDereferenceObject(settingsFileName);
                        }
                    }
                }
            }
        }

        // 3. Default location
        if (PhIsNullOrEmptyString(PhSettingsFileName))
        {
            PhSettingsFileName = PhGetKnownLocationZ(PH_FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.json", TRUE);
        }

        if (!PhIsNullOrEmptyString(PhSettingsFileName))
        {
            NTSTATUS status;

            status = PhLoadSettings(&PhSettingsFileName->sr);

            // If JSON file not found, try to convert from XML
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                PPH_STRING xmlFileName = PhGetBaseNameChangeExtensionZ(&PhSettingsFileName->sr, L".xml");

                if (!PhIsNullOrEmptyString(xmlFileName))
                {
                    if (PhDoesFileExist(&xmlFileName->sr))
                    {
                        // Convert XML to JSON
                        NTSTATUS convertStatus = PhConvertSettingsXmlToJson(
                            &xmlFileName->sr,
                            &PhSettingsFileName->sr
                            );

                        if (NT_SUCCESS(convertStatus))
                        {
                            // Retry loading from newly created JSON file
                            status = PhLoadSettings(&PhSettingsFileName->sr);
                        }
                    }

                    PhDereferenceObject(xmlFileName);
                }
            }

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
                    PhResetSettingsFile(&PhSettingsFileName->sr);
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
    PhPluginsEnabled = !!PhGetIntegerSetting(SETTING_ENABLE_PLUGINS);
    PhMaxSizeUnit = PhGetIntegerSetting(SETTING_MAX_SIZE_UNIT);
    PhMaxPrecisionUnit = (USHORT)PhGetIntegerSetting(SETTING_MAX_PRECISION_UNIT);
    PhMaxPrecisionLimit = 1.0f;
    for (ULONG i = 0; i < PhMaxPrecisionUnit; i++)
        PhMaxPrecisionLimit /= 10;
    PhEnableWindowText = !!PhGetIntegerSetting(SETTING_ENABLE_WINDOW_TEXT);

    PhEnableThemeSupport = !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT);
    PhThemeWindowForegroundColor = PhGetIntegerSetting(SETTING_THEME_WINDOW_FOREGROUND_COLOR);
    PhThemeWindowBackgroundColor = PhGetIntegerSetting(SETTING_THEME_WINDOW_BACKGROUND_COLOR);
    PhThemeWindowBackground2Color = PhGetIntegerSetting(SETTING_THEME_WINDOW_BACKGROUND2_COLOR);
    PhThemeWindowHighlightColor = PhGetIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT_COLOR);
    PhThemeWindowHighlight2Color = PhGetIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT2_COLOR);
    PhThemeWindowTextColor = PhGetIntegerSetting(SETTING_THEME_WINDOW_TEXT_COLOR);
    PhEnableThemeAcrylicSupport = WindowsVersion >= WINDOWS_11 && !!PhGetIntegerSetting(SETTING_ENABLE_THEME_ACRYLIC_SUPPORT);
    PhEnableThemeAcrylicWindowSupport = WindowsVersion >= WINDOWS_11 && !!PhGetIntegerSetting(SETTING_ENABLE_THEME_ACRYLIC_WINDOW_SUPPORT);
    PhEnableThemeNativeButtons = !!PhGetIntegerSetting(SETTING_ENABLE_THEME_NATIVE_BUTTONS);
    PhEnableThemeListviewBorder = !!PhGetIntegerSetting(SETTING_TREE_LIST_BORDER_ENABLE);
    PhEnableDeferredLayout = !!PhGetIntegerSetting(SETTING_ENABLE_DEFERRED_LAYOUT);
    PhEnableServiceNonPoll = !!PhGetIntegerSetting(SETTING_ENABLE_SERVICE_NON_POLL);
    PhEnableServiceNonPollNotify = !!PhGetIntegerSetting(SETTING_ENABLE_SERVICE_NON_POLL_NOTIFY);
    PhServiceNonPollFlushInterval = PhGetIntegerSetting(SETTING_NON_POLL_FLUSH_INTERVAL);
    PhEnableKsiSupport = !!PhGetIntegerSetting(SETTING_KSI_ENABLE) && !PhStartupParameters.NoKph && !PhIsExecutingInWow64();
    PhEnableKsiWarnings = !!PhGetIntegerSetting(SETTING_KSI_ENABLE_WARNINGS);
    PhFontQuality = PhGetFontQualitySetting(PhGetIntegerSetting(SETTING_FONT_QUALITY));

    if (PhGetIntegerSetting(SETTING_SAMPLE_COUNT_AUTOMATIC))
    {
        ULONG sampleCount;

        sampleCount = (PhGetSystemMetrics(SM_CXVIRTUALSCREEN, 0) + 1) / 2;

        if (sampleCount > 4096)
            sampleCount = 4096;

        PhSetIntegerSetting(SETTING_SAMPLE_COUNT, sampleCount);
    }

    if (PhStartupParameters.UpdateChannel != PhInvalidChannel)
    {
        PhSetIntegerSetting(SETTING_RELEASE_CHANNEL, PhStartupParameters.UpdateChannel);
    }

    if (PhStartupParameters.ShowHidden && !PhNfIconsEnabled())
    {
        // HACK(jxy-s) The default used to be that system tray icons where enabled, this keeps the
        // old behavior for automation workflows. If the user specified "-hide" then they want to
        // start the program hidden to the system tray and not show any main window. If there are no
        // system tray icons enabled then we need to enable them so the behavior is consistent.
        PhSetStringSetting(SETTING_ICON_SETTINGS, L"2|1");
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

/**
 * Callback function for processing command line options.
 *
 * This function is invoked for each parsed command line option during application startup.
 * It updates the global PhStartupParameters structure and related state based on the provided
 * option and value. Supported options include settings file path, visibility, debug mode,
 * privilege elevation, plugin parameters, process priority, and more.
 *
 * \param Option Pointer to the command line option structure, or NULL for non-option arguments.
 * \param Value Optional value associated with the option, or NULL if not applicable.
 * \param Context Optional context pointer (unused).
 * \return TRUE to continue processing further options, or FALSE to stop.
 */
_Function_class_(PH_COMMAND_LINE_CALLBACK)
BOOLEAN NTAPI PhpCommandLineOptionCallback(
    _In_opt_ PCPH_COMMAND_LINE_OPTION Option,
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

/**
 * Parses and processes the application's startup parameters from the command line.
 *
 * This function defines and processes all supported command line options for System Informer.
 * It updates the global PhStartupParameters structure based on the parsed arguments, enabling
 * or disabling features, setting file paths, and configuring application behavior.
 */
VOID PhpProcessStartupParameters(
    VOID
    )
{
    CONST PH_COMMAND_LINE_OPTION options[] =
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
}

/**
 * Enables a set of privileges for the current process token.
 *
 * \remarks This function is called during application startup to ensure
 * the process has the necessary rights for system-level operations.
 * If the process lacks sufficient rights to adjust privileges, the function
 * will silently fail and continue.
 */
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
