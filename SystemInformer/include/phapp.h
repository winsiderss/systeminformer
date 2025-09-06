/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2024
 *
 */

#ifndef PHAPP_H
#define PHAPP_H

#if !defined(_PHAPP_)
#define PHAPPAPI __declspec(dllimport)
#else
#define PHAPPAPI __declspec(dllexport)
#endif

#include <ph.h>
#include <guisup.h>
#include <provider.h>
#include <filestream.h>
#include <fastlock.h>
#include <treenew.h>
#include <graph.h>
#include <circbuf.h>
#include <dltmgr.h>
#include <phnet.h>

#include "../resource.h"

#include <phfwddef.h>
#include <appsup.h>
#include <searchbox.h>

// main

typedef struct _PH_STARTUP_PARAMETERS
{
    union
    {
        struct
        {
            ULONG NoSettings : 1;
            ULONG ShowVisible : 1;
            ULONG ShowHidden : 1;
            ULONG NoKph : 1;
            ULONG Debug : 1;
            ULONG ShowOptions : 1;
            ULONG PhSvc : 1;
            ULONG NoPlugins : 1;
            ULONG NewInstance : 1;
            ULONG Elevate : 1;
            ULONG Silent : 1;
            ULONG Help : 1;
            ULONG KphStartupHigh : 1;
            ULONG KphStartupMax : 1;
            ULONG Spare : 18;
        };
        ULONG Flags;
    };

    PPH_STRING SettingsFileName;
    PPH_STRING RunAsServiceMode;

    HWND WindowHandle;
    POINT Point;

    ULONG SelectPid;
    ULONG PriorityClass;

    PPH_LIST PluginParameters;
    PPH_STRING SelectTab;
    PPH_STRING SysInfo;
    PH_RELEASE_CHANNEL UpdateChannel;
} PH_STARTUP_PARAMETERS, *PPH_STARTUP_PARAMETERS;

extern BOOLEAN PhPluginsEnabled;
extern BOOLEAN PhPortableEnabled;
extern PPH_STRING PhSettingsFileName;
extern PH_STARTUP_PARAMETERS PhStartupParameters;

extern PH_PROVIDER_THREAD PhPrimaryProviderThread;
extern PH_PROVIDER_THREAD PhSecondaryProviderThread;
extern PH_PROVIDER_THREAD PhTertiaryProviderThread;

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhRegisterDialog(
    _In_ HWND DialogWindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhUnregisterDialog(
    _In_ HWND DialogWindowHandle
    );

typedef BOOLEAN (NTAPI *PPH_MESSAGE_LOOP_FILTER)(
    _In_ PMSG Message,
    _In_ PVOID Context
    );

typedef struct _PH_MESSAGE_LOOP_FILTER_ENTRY
{
    PPH_MESSAGE_LOOP_FILTER Filter;
    PVOID Context;
} PH_MESSAGE_LOOP_FILTER_ENTRY, *PPH_MESSAGE_LOOP_FILTER_ENTRY;

PHAPPAPI
PPH_MESSAGE_LOOP_FILTER_ENTRY
NTAPI
PhRegisterMessageLoopFilter(
    _In_ PPH_MESSAGE_LOOP_FILTER Filter,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhUnregisterMessageLoopFilter(
    _In_ PPH_MESSAGE_LOOP_FILTER_ENTRY FilterEntry
    );
// end_phapppub

// plugin

extern PH_AVL_TREE PhPluginsByName;

VOID PhInitializeCallbacks(
    VOID
    );

BOOLEAN PhIsPluginDisabled(
    _In_ PCPH_STRINGREF BaseName
    );

VOID PhSetPluginDisabled(
    _In_ PCPH_STRINGREF BaseName,
    _In_ BOOLEAN Disable
    );

VOID PhLoadPlugins(
    VOID
    );

VOID PhUnloadPlugins(
    _In_ BOOLEAN SessionEnding
    );

// log

#define PH_LOG_ENTRY_PROCESS_FIRST 1
#define PH_LOG_ENTRY_PROCESS_CREATE 1
#define PH_LOG_ENTRY_PROCESS_DELETE 2
#define PH_LOG_ENTRY_PROCESS_LAST 2

#define PH_LOG_ENTRY_SERVICE_FIRST 3
#define PH_LOG_ENTRY_SERVICE_CREATE 3
#define PH_LOG_ENTRY_SERVICE_DELETE 4
#define PH_LOG_ENTRY_SERVICE_START 5
#define PH_LOG_ENTRY_SERVICE_STOP 6
#define PH_LOG_ENTRY_SERVICE_CONTINUE 7
#define PH_LOG_ENTRY_SERVICE_PAUSE 8
#define PH_LOG_ENTRY_SERVICE_MODIFIED 9
#define PH_LOG_ENTRY_SERVICE_LAST 10

#define PH_LOG_ENTRY_DEVICE_REMOVED 11
#define PH_LOG_ENTRY_DEVICE_ARRIVED 12

#define PH_LOG_ENTRY_MESSAGE 100 // phapppub

typedef struct _PH_LOG_ENTRY *PPH_LOG_ENTRY; // phapppub

typedef struct _PH_LOG_ENTRY
{
    UCHAR Type;
    UCHAR Reserved1;
    USHORT Flags;
    LARGE_INTEGER Time;
    union
    {
        struct
        {
            HANDLE ProcessId;
            PPH_STRING Name;
            HANDLE ParentProcessId;
            PPH_STRING ParentName;
            NTSTATUS ExitStatus;
        } Process;
        struct
        {
            PPH_STRING Name;
            PPH_STRING DisplayName;
        } Service;
        struct
        {
            PPH_STRING Classification;
            PPH_STRING Name;
        } Device;
        PPH_STRING Message;
    };
    UCHAR Buffer[1];
} PH_LOG_ENTRY, *PPH_LOG_ENTRY;

extern PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;

VOID PhLogInitialization(
    VOID
    );

VOID PhClearLogEntries(
    VOID
    );

VOID PhLogProcessEntry(
    _In_ UCHAR Type,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING Name,
    _In_opt_ HANDLE ParentProcessId,
    _In_opt_ PPH_STRING ParentName,
    _In_opt_ ULONG Status
    );

VOID PhLogServiceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName
    );

VOID PhLogDeviceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhLogMessageEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    );

PHAPPAPI
PPH_STRING
NTAPI
PhFormatLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    );
// end_phapppub

// dbgcon

VOID PhShowDebugConsole(
    VOID
    );

// itemtips

PPH_STRING PhGetProcessTooltipText(
    _In_ PPH_PROCESS_ITEM Process,
    _Out_opt_ PULONG64 ValidToTickCount
    );

PPH_STRING PhGetServiceTooltipText(
    _In_ PPH_SERVICE_ITEM Service
    );

// cmdmode

NTSTATUS PhCommandModeStart(
    VOID
    );

// anawait

VOID PhUiAnalyzeWaitThread(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

// mdump

VOID PhUiCreateDumpFileProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ MINIDUMP_TYPE DumpType
    );

VOID PhShowCreateDumpFileProcessDialog(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// about

VOID PhShowAboutDialog(
    _In_ HWND ParentWindowHandle
    );

PPH_STRING PhGetDiagnosticsString(
    VOID
    );

PPH_STRING PhGetApplicationVersionString(
    _In_ BOOLEAN LinkToCommit
    );

// affinity

VOID PhShowProcessAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_THREAD_ITEM ThreadItem
    );

// begin_phapppub
_Success_(return)
PHAPPAPI
BOOLEAN
NTAPI
PhShowProcessAffinityDialog2(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PKAFFINITY NewAffinityMask
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemAffinityMask(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ KAFFINITY AffinityMask
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemPagePriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG PagePriority
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemIoPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ IO_PRIORITY_HINT IoPriority
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ UCHAR PriorityClass
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemPriorityBoost(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN PriorityBoost
    );

PHAPPAPI
NTSTATUS
NTAPI
PhSetProcessItemThrottlingState(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN ClearThrottlingState
    );
// end_phapppub

PHAPPAPI
VOID
NTAPI
PhShowThreadAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

// chcol

#define PH_CONTROL_TYPE_TREE_NEW 1

VOID PhShowChooseColumnsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HWND ControlHandle,
    _In_ ULONG Type
    );

// chdlg

// begin_phapppub
#define PH_CHOICE_DIALOG_SAVED_CHOICES 10

#define PH_CHOICE_DIALOG_CHOICE 0x0
#define PH_CHOICE_DIALOG_USER_CHOICE 0x1
#define PH_CHOICE_DIALOG_PASSWORD 0x2
#define PH_CHOICE_DIALOG_TYPE_MASK 0x3

PHAPPAPI
BOOLEAN
NTAPI
PhaChoiceDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Title,
    _In_ PCWSTR Message,
    _In_opt_ PCWSTR *Choices,
    _In_opt_ ULONG NumberOfChoices,
    _In_opt_ PCWSTR Option,
    _In_ ULONG Flags,
    _Inout_ PPH_STRING *SelectedChoice,
    _Inout_opt_ PBOOLEAN SelectedOption,
    _In_opt_ PCWSTR SavedChoicesSettingName
    );

PHAPPAPI
BOOLEAN
NTAPI
PhChoiceDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Title,
    _In_ PCWSTR Message,
    _In_opt_ PCWSTR* Choices,
    _In_opt_ ULONG NumberOfChoices,
    _In_opt_ PCWSTR Option,
    _In_ ULONG Flags,
    _Inout_ PPH_STRING* SelectedChoice,
    _Inout_opt_ PBOOLEAN SelectedOption,
    _In_opt_ PCWSTR SavedChoicesSettingName
    );
// end_phapppub

// chproc

// begin_phapppub
_Success_(return)
PHAPPAPI
BOOLEAN
NTAPI
PhShowChooseProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Message,
    _Out_ PHANDLE ProcessId
    );
// end_phapppub

// findobj

VOID PhShowFindObjectsDialog(
    _In_ HWND ParentWindowHandle
    );

// gdihndl

VOID PhShowGdiHandlesDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// heapinfo

VOID PhShowProcessHeapsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhShowProcessLocksDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// hidnproc

VOID PhShowZombieProcessesDialog(
    VOID
    );

// hndlprp

VOID PhShowHandleProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM HandleItem
    );

// hndlstat

VOID PhShowHandleStatisticsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId
    );

// infodlg

VOID PhShowInformationDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR String,
    _Reserved_ ULONG Flags
    );

// jobprp

VOID PhShowJobProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_opt_ PCWSTR Title
    );

HPROPSHEETPAGE PhCreateJobPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_opt_ DLGPROC HookProc
    );

// kdump

typedef union _PH_LIVE_DUMP_OPTIONS
{
    BOOLEAN Flags;
    struct
    {
        BOOLEAN CompressMemoryPages : 1;
        BOOLEAN IncludeUserSpaceMemory : 1;
        BOOLEAN IncludeHypervisorPages : 1;
        BOOLEAN OnlyKernelThreadStacks : 1;
        BOOLEAN UseDumpStorageStack : 1;
        BOOLEAN IncludeNonEssentialHypervisorPages : 1;
        BOOLEAN Spare : 2;
    };
} PH_LIVE_DUMP_OPTIONS, *PPH_LIVE_DUMP_OPTIONS;

VOID PhUiCreateLiveDump(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_LIVE_DUMP_OPTIONS Options
    );

VOID PhShowLiveDumpDialog(
    _In_ HWND ParentWindowHandle
    );

// ksyscall

PPH_STRING PhGetSystemCallNumberName(
    _In_ USHORT SystemCallNumber
    );

// logwnd

VOID PhShowLogDialog(
    VOID
    );

// memedit

#define PH_MEMORY_EDITOR_UNMAP_VIEW_OF_SECTION 0x1

VOID PhShowMemoryEditorDialog(
    _In_ HWND OwnerWindow,
    _In_ HANDLE ProcessId,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG SelectOffset,
    _In_ ULONG SelectLength,
    _In_opt_ PPH_STRING Title,
    _In_ ULONG Flags
    );

// memlists

VOID PhShowMemoryListsDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ VOID (NTAPI *RegisterDialog)(HWND),
    _In_opt_ VOID (NTAPI *UnregisterDialog)(HWND)
    );

// memprot

VOID PhShowMemoryProtectDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MEMORY_ITEM MemoryItem
    );

// memrslt

VOID PhShowMemoryResultsDialog(
    _In_ HANDLE ProcessId,
    _In_ PPH_LIST Results
    );

// memsrch

VOID PhShowMemoryStringDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// msmsrcht

VOID PhShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// memmod

VOID PhShowImagePageModifiedDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// mtgndlg

VOID PhShowProcessMitigationPolicyDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId
    );

// ntobjprp

HPROPSHEETPAGE PhCreateEventPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    );

HPROPSHEETPAGE PhCreateEventPairPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    );

HPROPSHEETPAGE PhCreateSemaphorePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    );

HPROPSHEETPAGE PhCreateTimerPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    );

HPROPSHEETPAGE PhCreateAfdSocketPage(
    _In_ HANDLE ProcessId,
    _In_ HANDLE HandleValue
    );

HPROPSHEETPAGE PhCreateMappingsPage(
    _In_ HANDLE ProcessId,
    _In_ HANDLE SectionHandle
    );

// options

VOID PhShowOptionsDialog(
    _In_ HWND ParentWindowHandle
    );

// pagfiles

VOID PhShowPagefilesDialog(
    _In_ HWND ParentWindowHandle
    );

// plugman

INT_PTR CALLBACK PhPluginsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// procrec

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhShowProcessRecordDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_RECORD Record
    );
// end_phapppub

// runas

typedef struct _PH_RUNAS_SERVICE_PARAMETERS
{
    ULONG ProcessId;
    PCWSTR UserName;
    PCWSTR Password;
    ULONG LogonType;
    ULONG SessionId;
    PCWSTR CurrentDirectory;
    PCWSTR CommandLine;
    PCWSTR FileName;
    PCWSTR DesktopName;
    BOOLEAN UseLinkedToken;
    PCWSTR ServiceName;
    BOOLEAN CreateSuspendedProcess;
    HWND WindowHandle;
    BOOLEAN CreateUIAccessProcess;
} PH_RUNAS_SERVICE_PARAMETERS, *PPH_RUNAS_SERVICE_PARAMETERS;

VOID PhShowRunAsDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ HANDLE ProcessId
    );

VOID PhShowRunAsPackageDialog(
    _In_ HWND ParentWindowHandle
    );

// begin_phapppub
PHLIBAPI
BOOLEAN
NTAPI
PhShowRunFileDialog(
    _In_ HWND ParentWindowHandle
    );
// end_phapppub

NTSTATUS PhExecuteRunAsCommand(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

// begin_phapppub
PHAPPAPI
NTSTATUS
NTAPI
PhExecuteRunAsCommand2(
    _In_ HWND hWnd,
    _In_ PCWSTR Program,
    _In_opt_ PCWSTR UserName,
    _In_opt_ PCWSTR Password,
    _In_opt_ ULONG LogonType,
    _In_opt_ HANDLE ProcessIdWithToken,
    _In_opt_ ULONG SessionId,
    _In_opt_ PCWSTR DesktopName,
    _In_ BOOLEAN UseLinkedToken
    );
// end_phapppub

PHAPPAPI
NTSTATUS
NTAPI
PhExecuteRunAsCommand3(
    _In_ HWND hWnd,
    _In_ PCWSTR Program,
    _In_opt_ PCWSTR UserName,
    _In_opt_ PCWSTR Password,
    _In_opt_ ULONG LogonType,
    _In_opt_ HANDLE ProcessIdWithToken,
    _In_opt_ ULONG SessionId,
    _In_opt_ PCWSTR DesktopName,
    _In_ BOOLEAN UseLinkedToken,
    _In_ BOOLEAN CreateSuspendedProcess,
    _In_ BOOLEAN CreateUIAccessProcess
    );

NTSTATUS PhRunAsServiceStart(
    _In_ PPH_STRING ServiceName
    );

NTSTATUS PhInvokeRunAsService(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

// searchbox

// begin_phapppub
typedef _Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PH_SEARCHCONTROL_CALLBACK(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    );
typedef PH_SEARCHCONTROL_CALLBACK* PPH_SEARCHCONTROL_CALLBACK;

PHAPPAPI
VOID
NTAPI
PhCreateSearchControl(
    _In_ HWND ParentWindowHandle,
    _In_ HWND SearchWindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    );
// end_phapppub

// sessmsg

VOID PhShowSessionSendMessageDialog(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    );

// sessprp

VOID PhShowSessionProperties(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    );

// sessshad

VOID PhShowSessionShadowDialog(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    );

// srvcr

VOID PhShowCreateServiceDialog(
    _In_ HWND ParentWindowHandle
    );

// srvctl

// begin_phapppub
#define WM_PH_SET_LIST_VIEW_SETTINGS (WM_APP + 701)

PHAPPAPI
HWND
NTAPI
PhCreateServiceListControl(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    );
// end_phapppub

// srvprp

VOID PhShowServiceProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

// thrdstk

VOID PhShowThreadStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

// thrdstks

VOID PhShowThreadStacksDialog(
    _In_ HWND ParentWindowHandle
    );

// userslist

VOID PhShowUserListDialog(
    _In_ HWND ParentWindowHandle
    );

// tokprp

PPH_STRING PhGetGroupAttributesString(
    _In_ ULONG Attributes,
    _In_ BOOLEAN Restricted
    );

PCWSTR PhGetPrivilegeAttributesString(
    _In_ ULONG Attributes
    );

_Success_(return)
BOOLEAN PhGetElevationTypeString(
    _In_ BOOLEAN IsElevated,
    _In_ TOKEN_ELEVATION_TYPE ElevationType,
    _Out_ PCPH_STRINGREF* ElevationTypeString
    );

VOID PhShowTokenProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_ HANDLE ProcessId,
    _In_ PVOID Context,
    _In_opt_ PCWSTR Title
    );

HPROPSHEETPAGE PhCreateTokenPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_ HANDLE ProcessId,
    _In_opt_ PVOID Context,
    _In_opt_ DLGPROC HookProc
    );

#endif
